#include "tape_service.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "database.hpp"
#include "delete_response.hpp"
#include "readytakeover_response.hpp"
#include "release_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "storage.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include <string>

namespace storm {

StageResponse TapeService::stage(StageRequest stage_request)
{
  auto& files = stage_request.files;
  // de-duplication is needed because the path is a primary key of the db
  std::sort(files.begin(), files.end(),
            [](File const& a, File const& b) { return a.path < b.path; });
  files.erase(std::unique(files.begin(), files.end(),
                          [](File const& a, File const& b) {
                            return a.path == b.path;
                          }),
              files.end());

  for (auto& file : files) {
    std::error_code ec;
    auto status = fs::status(file.path, ec);
    if (ec || !fs::is_regular_file(status)) {
      file.state = File::State::failed;
    }
  }
  auto const uuid     = m_uuid_gen();
  auto const id       = to_string(uuid);
  auto const inserted = m_db->insert(id, stage_request);
  return inserted ? StageResponse{id} : StageResponse{};
}

StatusResponse TapeService::status(StageId const& id)
{
  auto maybe_stage = m_db->find(id);

  // FIXME: what about StatusResponse with id but stage value is empty?

  if (!maybe_stage.has_value()) {
    return StatusResponse{};
  }

  // update each file locality
  auto& stage = maybe_stage.value();
  auto& files = stage.files;
  for (auto& file : files) {
    switch (file.state) {
    case File::State::completed:
      file.locality = Locality::disk; // even if it can be disk_and_tape
      break;
    case File::State::started: {
      // TODO is this the way to know if a file has been staged?
      auto const locality = m_storage->locality(file.path);
      if (locality == Locality::disk || locality == Locality::disk_and_tape) {
        file.state    = File::State::completed;
        file.locality = Locality::disk; // even if it can be disk_and_tape
        // TODO update state to completed in the db
      }
      break;
    }
    case File::State::cancelled:
    case File::State::failed:
    case File::State::submitted:
      // do nothing
      break;
    }
  }

  return StatusResponse{id, std::move(stage)};
}

CancelResponse TapeService::cancel(StageId const& id, CancelRequest cancel)
{
  auto stage = m_db->find(id);
  if (!stage.has_value()) {
    return CancelResponse{};
  }

  auto proj = [](File const& stage_file) -> Path const& {
    return stage_file.path;
  };

  Paths invalid{};
  std::set_difference(
      cancel.paths.begin(), cancel.paths.end(),
      boost::make_transform_iterator(stage->files.begin(), proj),
      boost::make_transform_iterator(stage->files.end(), proj),
      std::back_inserter(invalid));

  if (!invalid.empty()) {
    return CancelResponse{id, std::move(invalid)};
  }

  for (auto& path : cancel.paths) {
    m_db->update(id, path, File::State::cancelled);
  }

  return CancelResponse{id};
}

DeleteResponse TapeService::erase(StageId const& id)
{
  // TODO cancel file stage requests?
  auto const erased = m_db->erase(id);
  return DeleteResponse{erased};
}

ReleaseResponse TapeService::release(StageId const& id, ReleaseRequest release)
{
  auto stage = m_db->find(id);
  if (!stage.has_value()) {
    return ReleaseResponse{};
  }

  auto proj = [](File const& stage_file) -> Path const& {
    return stage_file.path;
  };

  Paths both;
  both.reserve(release.paths.size());
  std::set_intersection(
      release.paths.begin(), release.paths.end(),
      boost::make_transform_iterator(stage->files.begin(), proj),
      boost::make_transform_iterator(stage->files.end(), proj),
      std::back_inserter(both));

  if (release.paths.size() != both.size()) {
    Paths invalid;
    assert(release.paths.size() > both.size());
    invalid.reserve(release.paths.size() - both.size());
    std::set_difference(release.paths.begin(), release.paths.end(),
                        both.begin(), both.end(), std::back_inserter(invalid));
    return ReleaseResponse{id, invalid};
  }

  // TODO do something?

  return ReleaseResponse{};
}

ArchiveInfoResponse TapeService::archive_info(ArchiveInfoRequest info)
{
  PathInfos infos;
  auto& paths = info.paths;
  infos.reserve(paths.size());
  std::transform( //
      paths.begin(), paths.end(), std::back_inserter(infos), [&](Path& path) {
        using namespace std::string_literals;
        std::error_code ec;
        auto status = fs::status(path, ec);
        if (ec) {
          return PathInfo{std::move(path), Locality::unavailable};
        }
        if (!fs::exists(status)) {
          return PathInfo{std::move(path), "path doesn't exist"s};
        }
        if (fs::is_directory(status)) {
          return PathInfo{std::move(path), "path is a directory"s};
        }
        if (!fs::is_regular_file(status)) {
          return PathInfo{std::move(path), "path is not a regular file"s};
        }
        auto const loc = m_storage->locality(path);
        return PathInfo{std::move(path), loc};
      });

  return ArchiveInfoResponse{infos};
}

ReadyTakeOverResponse TapeService::ready_take_over()
{
  auto const n = m_db->count_files(File::State::submitted);
  return ReadyTakeOverResponse{n};
}

TakeOverResponse TapeService::take_over(TakeOverRequest req)
{
  auto files = m_db->get_files(File::State::submitted, req.n_files);
  std::vector<std::pair<std::string, Locality>> file_localities;
  file_localities.reserve(files.size());
  for (auto&& file : files) {
    Path const p{file};
    file_localities.emplace_back(std::move(file), m_storage->locality(p));
  }

  auto it = std::partition(
      file_localities.begin(), file_localities.end(),
      [&](auto const& file_loc) { return file_loc.second == Locality::tape; });

  auto const now = std::time(nullptr);

  // update the state of files already on disk to Completed
  std::for_each(it, file_localities.end(), [&](auto const& file_loc) {
    m_db->update(file_loc.first, File::State::completed, now);
  });

  // don't pass files already on disk to GEMSS
  file_localities.erase(it, file_localities.end());

  // update the state of files to be passed to GEMSS to Started
  // and restore the original vector of filenames
  files.clear();
  files.reserve(file_localities.size());
  for (auto const& [file, loc] : file_localities) {
    m_db->update(Path{file}, File::State::started, now);
    files.push_back(std::move(file));
  }

  return TakeOverResponse{std::move(files)};
}

} // namespace storm
