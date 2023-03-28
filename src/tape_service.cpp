#include "tape_service.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "configuration.hpp"
#include "database.hpp"
#include "delete_response.hpp"
#include "extended_attributes.hpp"
#include "readytakeover_response.hpp"
#include "release_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "storage.hpp"
#include "storage_area_resolver.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include <fmt/core.h>
#include <span>
#include <string>

namespace storm {

StageResponse TapeService::stage(StageRequest stage_request)
{
  auto& files = stage_request.files;
  // de-duplication is needed because the path is a primary key of the db
  std::sort(files.begin(), files.end(), [](File const& a, File const& b) {
    return a.logical_path < b.logical_path;
  });
  files.erase(std::unique(files.begin(), files.end(),
                          [](File const& a, File const& b) {
                            return a.logical_path == b.logical_path;
                          }),
              files.end());

  StorageAreaResolver resolve{m_config.storage_areas};
  for (auto& file : files) {
    file.physical_path = resolve(file.logical_path);
    std::error_code ec;
    auto status = fs::status(file.physical_path, ec);
    if (ec || !fs::is_regular_file(status)) {
      file.state = File::State::failed;
    }
  }
  auto const uuid     = m_uuid_gen();
  auto const id       = to_string(uuid);
  auto const inserted = m_db->insert(id, stage_request);
  if (!inserted) {
    CROW_LOG_ERROR << fmt::format(
        "Failed to insert request {} into the database", id);
  }
  return inserted ? StageResponse{id} : StageResponse{};
}

static bool recall_in_progress(Path const& path)
{
  XAttrName const tsm_rect{"user.TSMRecT"};
  std::error_code ec;
  auto const in_progress = has_xattr(path, tsm_rect, ec);
  return ec == std::error_code{} && in_progress;
}

StatusResponse TapeService::status(StageId const& id)
{
  auto maybe_stage = m_db->find(id);

  if (!maybe_stage.has_value()) {
    return StatusResponse{};
  }

  // update each file locality
  auto& stage = maybe_stage.value();
  for (auto& files = stage.files; auto& file : files) {
    switch (file.state) {
    case File::State::completed:
      file.locality = Locality::disk; // even if it can be disk_and_tape
      break;
    case File::State::started: {
      if (recall_in_progress(file.physical_path)) {
        break;
      }
      auto locality = m_storage->locality(file.physical_path);
      if (locality == Locality::lost) {
        CROW_LOG_ERROR << fmt::format(
            "The file {} appears lost, check stubbification and presence of "
            "user.storm.migrated xattr",
            file.physical_path.string());
        // do not scare the client
        locality = Locality::unavailable;
      }
      auto const on_disk =
          locality == Locality::disk || locality == Locality::disk_and_tape;
      file.state    = on_disk ? File::State::completed : File::State::failed;
      file.locality = locality;
      m_db->update(file.physical_path, file.state, std::time(nullptr));
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
    return stage_file.logical_path;
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

  for (auto const& path : cancel.paths) {
    m_db->update(id, path, File::State::cancelled);
  }

  // do not bother cancelling the recalls in progress

  return CancelResponse{id};
}

DeleteResponse TapeService::erase(StageId const& id)
{
  // do not bother cancelling the recalls in progress

  auto const erased = m_db->erase(id);
  return DeleteResponse{erased};
}

ReleaseResponse TapeService::release(StageId const& id,
                                     ReleaseRequest release) const
{
  auto stage = m_db->find(id);
  if (!stage.has_value()) {
    return ReleaseResponse{};
  }

  auto proj = [](File const& stage_file) -> Path const& {
    return stage_file.logical_path;
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

  // do nothing

  return ReleaseResponse{};
}

ArchiveInfoResponse TapeService::archive_info(ArchiveInfoRequest info)
{
  PathInfos infos;
  auto& paths = info.paths;
  infos.reserve(paths.size());

  StorageAreaResolver resolve{m_config.storage_areas};

  std::transform( //
      paths.begin(), paths.end(), std::back_inserter(infos),
      [&](Path& logical_path) {
        using namespace std::string_literals;

        auto const physical_path = resolve(logical_path);
        std::error_code ec;
        auto status = fs::status(physical_path, ec);

        // if the file doesn't exist, fs::status sets ec, so check first for
        // existence
        if (!fs::exists(status)) {
          return PathInfo{std::move(logical_path),
                          "No such file or directory"s};
        }
        if (ec != std::error_code{}) {
          return PathInfo{std::move(logical_path), Locality::unavailable};
        }
        if (fs::is_directory(status)) {
          return PathInfo{std::move(logical_path), "Is a directory"s};
        }
        if (!fs::is_regular_file(status)) {
          return PathInfo{std::move(logical_path), "Not a regular file"s};
        }
        auto locality = m_storage->locality(physical_path);
        if (locality == Locality::lost) {
          CROW_LOG_ERROR << fmt::format("The file {} appears lost",
                                        logical_path.string());
          // do not scare the client
          locality = Locality::unavailable;
        }
        return PathInfo{std::move(logical_path), locality};
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

  std::vector<std::pair<Path, Locality>> path_localities;
  path_localities.reserve(files.size());
  for (auto&& file : files) {
    Path p{std::move(file)};
    auto const locality{m_storage->locality(p)};
    path_localities.emplace_back(std::move(p), locality);
  }

  auto const it = std::partition(path_localities.begin(), path_localities.end(),
                                 [&](auto const& file_loc) {
                                   return file_loc.second == Locality::tape
                                       || file_loc.second == Locality::lost;
                                 });

  std::span const on_tape_or_lost{path_localities.begin(), it};

  auto const it2 =
      std::partition(it, path_localities.end(), [](auto const& file_loc) {
        return file_loc.second == Locality::disk
            || file_loc.second == Locality::disk_and_tape;
      });
  std::span const on_disk{it, it2};
  std::span const the_rest{it2, path_localities.end()};

  auto const now = std::time(nullptr);

  // update the state of files already on disk to Completed
  std::for_each(on_disk.begin(), on_disk.end(), [&](auto const& file_loc) {
    // started_at may remain at its default value
    m_db->update(file_loc.first, File::State::completed, now);
  });

  // update the state of all the other files (unavailable/none) to Failed
  std::for_each(the_rest.begin(), the_rest.end(), [&](auto const& file_loc) {
    // started_at may remain at its default value
    m_db->update(file_loc.first, File::State::failed, now);
  });

  // update the state of files to be passed to GEMSS to Started
  // let's try also presumably lost files
  Paths paths;
  paths.reserve(on_tape_or_lost.size());
  for (auto& [path, loc] : on_tape_or_lost) {
    // first set the xattr, then update the DB. failing to update the DB is not
    // a big deal, because the file stays in submitted state and can be passed
    // later again to GEMSS. passing a file to GEMSS is mostly an idempotent
    // operation
    XAttrName const tsm_rect{"user.TSMRecT"};
    std::error_code ec;
    create_xattr(path, tsm_rect, ec);
    if (ec == std::error_code{}) {
      m_db->update(path, File::State::started, now);
      paths.push_back(std::move(path));
    } else {
      CROW_LOG_WARNING << fmt::format("Cannot create xattr {} for file {}",
                                      tsm_rect.value(), path.string());
    }
  }

  return TakeOverResponse{std::move(paths)};
}

} // namespace storm
