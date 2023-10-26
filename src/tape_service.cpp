#include "tape_service.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "configuration.hpp"
#include "database.hpp"
#include "delete_response.hpp"
#include "errors.hpp"
#include "extended_attributes.hpp"
#include "io.hpp"
#include "profiler.hpp"
#include "readytakeover_response.hpp"
#include "release_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "storage.hpp"
#include "storage_area_resolver.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include <boost/iterator/transform_iterator.hpp>
#include <crow/logging.h>
#include <fmt/core.h>
#include <ctime>
#include <span>
#include <string>

namespace storm {

StageResponse TapeService::stage(StageRequest stage_request)
{
  PROFILE_FUNCTION();
  auto& files = stage_request.files;
  // de-duplication is needed because the logical path is a primary key of the
  // db
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
      file.state       = File::State::failed;
      file.started_at  = std::time(nullptr);
      file.finished_at = file.started_at;
    }
  }
  auto const uuid     = m_uuid_gen();
  auto const id       = to_string(uuid);
  auto const inserted = m_db->insert(id, stage_request);
  if (!inserted) {
    CROW_LOG_ERROR << fmt::format(
        "Failed to insert request {} into the database", id);
  }
  return inserted ? StageResponse{id, std::move(files)} : StageResponse{};
}

static bool recall_in_progress(PhysicalPath const& path)
{
  XAttrName const tsm_rect{"user.TSMRecT"};
  std::error_code ec;
  auto const in_progress = has_xattr(path, tsm_rect, ec);
  return ec == std::error_code{} && in_progress;
}

static bool override_locality(Locality& locality, PhysicalPath const& path)
{
  if (locality == Locality::lost) {
    CROW_LOG_ERROR << fmt::format(
        "The file {} appears lost, check stubbification and presence of "
        "user.storm.migrated xattr",
        path.string());
    // do not scare the client
    locality = Locality::unavailable;
    return true;
  }
  return false;
}

StatusResponse TapeService::status(StageId const& id)
{
  PROFILE_FUNCTION();
  auto maybe_stage = m_db->find(id);

  if (!maybe_stage.has_value()) {
    throw StageNotFound(id);
  }

  const auto now = std::time(nullptr);

  // determine the actual state of files and update the db
  std::vector<std::pair<PhysicalPath, File::State>> files_to_update;
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
      override_locality(locality, file.physical_path);
      auto const on_disk =
          locality == Locality::disk || locality == Locality::disk_and_tape;
      file.state       = on_disk ? File::State::completed : File::State::failed;
      file.locality    = locality;
      file.finished_at = now;
      files_to_update.push_back({file.physical_path, file.state});
      break;
    }
    case File::State::cancelled:
    case File::State::failed:
      // do nothing
      break;
    case File::State::submitted: {
      if (recall_in_progress(file.physical_path)) {
        file.state      = File::State::started;
        file.started_at = now;
        files_to_update.push_back({file.physical_path, File::State::started});
      } else if (auto locality = m_storage->locality(file.physical_path);
                 locality != Locality::tape) {
        override_locality(locality, file.physical_path);
        file.locality = locality;
        file.state =
            file.on_disk() ? File::State::completed : File::State::failed;
        file.started_at  = now;
        file.finished_at = now;
        files_to_update.push_back({file.physical_path, file.state});
      }
      break;
    }
    }
  }

  std::sort(stage.files.begin(), stage.files.end(),
            [](auto const& f1, auto const& f2) {
              return to_underlying(f1.state) < to_underlying(f2.state);
            });

  auto const updated = stage.update_timestamps();
  StageUpdate stage_update{
      updated ? std::optional(StageEntity{id, stage.created_at,
                                          stage.started_at, stage.completed_at})
              : std::nullopt,
      files_to_update, now};
  m_db->update(stage_update);
  return StatusResponse{id, std::move(stage)};
}

CancelResponse TapeService::cancel(StageId const& id, CancelRequest cancel)
{
  PROFILE_FUNCTION();
  auto stage = m_db->find(id);

  if (!stage.has_value()) {
    throw StageNotFound{id};
  }

  auto proj = [](File const& stage_file) -> LogicalPath const& {
    return stage_file.logical_path;
  };

  std::sort(cancel.paths.begin(), cancel.paths.end());

  LogicalPaths invalid{};
  std::set_difference(
      cancel.paths.begin(), cancel.paths.end(),
      boost::make_transform_iterator(stage->files.begin(), proj),
      boost::make_transform_iterator(stage->files.end(), proj),
      std::back_inserter(invalid));

  if (!invalid.empty()) {
    return CancelResponse{id, std::move(invalid)};
  }

  const auto now = std::time(nullptr);
  m_db->update(id, cancel.paths, File::State::cancelled, now);
  // do not bother cancelling the recalls in progress

  return CancelResponse{id};
}

DeleteResponse TapeService::erase(StageId const& id)
{
  PROFILE_FUNCTION();
  // do not bother cancelling the recalls in progress
  auto const erased = m_db->erase(id);
  if (!erased) {
    throw StageNotFound(id);
  }
  return {};
}

ReleaseResponse TapeService::release(StageId const& id,
                                     ReleaseRequest release) const
{
  PROFILE_FUNCTION();
  auto stage = m_db->find(id);
  if (!stage.has_value()) {
    throw StageNotFound{id};
  }

  auto proj = [](File const& stage_file) -> LogicalPath const& {
    return stage_file.logical_path;
  };

  std::sort(release.paths.begin(), release.paths.end());

  LogicalPaths invalid{};
  std::set_difference(
      release.paths.begin(), release.paths.end(),
      boost::make_transform_iterator(stage->files.begin(), proj),
      boost::make_transform_iterator(stage->files.end(), proj),
      std::back_inserter(invalid));

  if (!invalid.empty()) {
    return ReleaseResponse{id, std::move(invalid)};
  }

  // do nothing

  return ReleaseResponse{id};
}

ArchiveInfoResponse TapeService::archive_info(ArchiveInfoRequest info)
{
  PROFILE_FUNCTION();
  PathInfos infos;
  auto& paths = info.paths;
  infos.reserve(paths.size());

  StorageAreaResolver resolve{m_config.storage_areas};

  std::transform( //
      paths.begin(), paths.end(), std::back_inserter(infos),
      [&](LogicalPath& logical_path) {
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
        override_locality(locality, physical_path);
        return PathInfo{std::move(logical_path), locality};
      });

  return ArchiveInfoResponse{infos};
}

ReadyTakeOverResponse TapeService::ready_take_over()
{
  PROFILE_FUNCTION();
  auto const n = m_db->count_files(File::State::submitted);
  return ReadyTakeOverResponse{n};
}

using PathLocality = std::pair<PhysicalPath, Locality>;

static auto extend_paths_with_localities(PhysicalPaths&& paths, Storage& storage)
{
  std::vector<PathLocality> path_localities;
  path_localities.reserve(paths.size());

  for (auto&& path : paths) {
    auto const locality{storage.locality(path)};
    path_localities.emplace_back(std::move(path), locality);
  }

  return path_localities;
}

static auto select_only_on_tape(std::span<PathLocality> path_locs)
{
  auto const it = std::partition(path_locs.begin(), path_locs.end(),
                                 [&](auto const& path_loc) {
                                   return path_loc.second == Locality::tape
                                       // let's try also apparently-lost files
                                       || path_loc.second == Locality::lost;
                                 });
  return std::tuple{std::span{path_locs.begin(), it},
                    std::span{it, path_locs.end()}};
}

static auto select_in_progress(std::span<PathLocality> path_locs)
{
  auto const it = std::partition(
      path_locs.begin(), path_locs.end(),
      [](auto const& path_loc) { return recall_in_progress(path_loc.first); });
  return std::tuple{std::span{path_locs.begin(), it},
                    std::span{it, path_locs.end()}};
}

static auto select_on_disk(std::span<PathLocality> path_locs)
{
  auto const it = std::partition(
      path_locs.begin(), path_locs.end(), [](auto const& path_loc) {
        return path_loc.second == Locality::disk
            || path_loc.second == Locality::disk_and_tape;
      });
  return std::tuple{std::span{path_locs.begin(), it},
                    std::span{it, path_locs.end()}};
}

TakeOverResponse TapeService::take_over(TakeOverRequest req)
{
  PROFILE_FUNCTION();
  auto physical_paths = m_db->get_files(File::State::submitted, req.n_files);

  auto path_locs =
      extend_paths_with_localities(std::move(physical_paths), *m_storage);

  auto [only_on_tape, not_only_on_tape] = select_only_on_tape(path_locs);
  auto [in_progress, need_recall]       = select_in_progress(only_on_tape);
  auto [on_disk, the_rest]              = select_on_disk(not_only_on_tape);

  auto const now = std::time(nullptr);

  auto proj = [](auto const& file_loc) { return file_loc.first; };

  // reuse physical_paths, premature optimization?
  // reserve enough space for all the following assignments
  physical_paths.reserve(path_locs.size());

  physical_paths.assign(
      boost::make_transform_iterator(in_progress.begin(), proj),
      boost::make_transform_iterator(in_progress.end(), proj));
  m_db->update(physical_paths, File::State::started, now);

  // update the state of files already on disk to Completed
  // started_at may remain at its default value
  physical_paths.assign(boost::make_transform_iterator(on_disk.begin(), proj),
                        boost::make_transform_iterator(on_disk.end(), proj));
  m_db->update(physical_paths, File::State::completed, now);

  // update the state of all the other files (unavailable/none) to Failed
  // started_at may remain at its default value
  physical_paths.assign(boost::make_transform_iterator(the_rest.begin(), proj),
                        boost::make_transform_iterator(the_rest.end(), proj));
  m_db->update(physical_paths, File::State::failed, now);

  // update the state of files to be passed to GEMSS to Started
  physical_paths.assign(
      boost::make_transform_iterator(need_recall.begin(), proj),
      boost::make_transform_iterator(need_recall.end(), proj));

  // first set the xattr, then update the DB. failing to update the DB is not
  // a big deal, because the file stays in submitted state and can be passed
  // later again to GEMSS. passing a file to GEMSS is mostly an idempotent
  // operation
  // clang-format off
  std::for_each(
      physical_paths.begin(), physical_paths.end(), [&](auto& physical_path) {
        XAttrName const tsm_rect{"user.TSMRecT"};
        std::error_code ec;
        create_xattr(physical_path, tsm_rect, ec);
        if (ec != std::error_code{}) {
          CROW_LOG_WARNING << fmt::format("Cannot create xattr {} for file {}",
                                          tsm_rect.value(),
                                          physical_path.string());
        }
      });
  // clang-format on
  m_db->update(physical_paths, File::State::started, now);

  return TakeOverResponse{std::move(physical_paths)};
}

} // namespace storm
