#include "tape_service.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "database.hpp"
#include "delete_response.hpp"
#include "release_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"

namespace storm {

StageResponse TapeService::stage(StageRequest const& stage_request)
{
  auto const uuid     = m_uuid_gen();
  auto const id       = to_string(uuid);
  auto const inserted = m_db->insert(id, stage_request);
  return inserted ? StageResponse{id} : StageResponse{};
}

StatusResponse TapeService::status(StageId const& id)
{
  auto stage = m_db->find(id);
  // FIXME: what about StatusResponse with id but stage value is empty?
  return stage.has_value() ? StatusResponse{id, stage.value()}
                           : StatusResponse{};
}

CancelResponse TapeService::cancel(StageId const& id,
                                   CancelRequest const& cancel)
{
  auto stage = m_db->find(id);
  if (!stage.has_value()) {
    return CancelResponse{};
  }

  auto proj = [](File const& stage_file) -> Path const& {
    return stage_file.path;
  };

  std::vector<std::filesystem::path> invalid{};
  std::set_difference(
      boost::make_transform_iterator(cancel.paths.begin(), proj),
      boost::make_transform_iterator(cancel.paths.end(), proj),
      boost::make_transform_iterator(stage->files().begin(), proj),
      boost::make_transform_iterator(stage->files().end(), proj),
      std::back_inserter(invalid));

  if (!invalid.empty()) {
    return CancelResponse{id, std::move(invalid)};
  }

  // TODO set the status of files to cancelled

  return CancelResponse{id};
}

DeleteResponse TapeService::erase(StageId const& id)
{
  // TODO cancel file stage requests?
  auto const erased = m_db->erase(id);
  return DeleteResponse{erased};
}

ReleaseResponse TapeService::release(StageId const& id,
                                     ReleaseRequest const& release)
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
      boost::make_transform_iterator(release.paths.begin(), proj),
      boost::make_transform_iterator(release.paths.end(), proj),
      boost::make_transform_iterator(stage->files().begin(), proj),
      boost::make_transform_iterator(stage->files().end(), proj),
      std::back_inserter(both));

  if (release.paths.size() != both.size()) {
    Paths invalid;
    assert(release.paths.size() > both.size());
    invalid.reserve(release.paths.size() - both.size());
    std::set_difference(
        boost::make_transform_iterator(release.paths.begin(), proj),
        boost::make_transform_iterator(release.paths.end(), proj), both.begin(),
        both.end(), std::back_inserter(invalid));
    return ReleaseResponse{id, invalid};
  } else {
    // .......DO SOMETHING?.......
    return ReleaseResponse{};
  }
}

ArchiveInfoResponse TapeService::archive(ArchiveInfoRequest const& /*info*/)
{
  // boost::json::array jbody;
  // Paths file_buffer;
  // file_buffer.reserve(m_id_buffer.size());

  // for (auto& id : m_id_buffer) {
  //   auto stage = m_db->find(id);
  //   if (!stage.has_value())
  //     continue;
  //   for (auto& file : stage->files()) {
  //     file_buffer.push_back(file.path);
  //   }
  // }

  // jbody.reserve(info.paths.size());

  // auto proj = [](File const& stage_file) -> Path const& {
  //   return stage_file.path;
  // };

  // Paths both;
  // both.reserve(info.paths.size());
  // std::set_intersection(
  //     boost::make_transform_iterator(info.paths.begin(), proj),
  //     boost::make_transform_iterator(info.paths.end(), proj),
  //     file_buffer.begin(), file_buffer.end(), std::back_inserter(both));

  // if (info.paths.size() != both.size()) {
  //   Paths invalid;
  //   assert(info.paths.size() > both.size());
  //   invalid.reserve(info.paths.size() - both.size());
  //   std::set_difference(
  //       boost::make_transform_iterator(info.paths.begin(), proj),
  //       boost::make_transform_iterator(info.paths.end(), proj), both.begin(),
  //       both.end(), std::back_inserter(invalid));

  //   jbody = not_in_archive_to_json(invalid, jbody);

  //   std::vector<File> remaining;
  //   for (auto& file : info.paths) {
  //     if (std::find(invalid.begin(), invalid.end(), file.path) !=
  //     invalid.end())
  //       continue;
  //     remaining.push_back(file);
  //   }

  //   jbody = archive_to_json(remaining, jbody);

  //   return ArchiveInfoResponse{jbody, invalid, remaining};
  // } else {
  //   jbody = archive_to_json(info.paths, jbody);

  //   return ArchiveInfoResponse{jbody, info.paths};
  // }
  return ArchiveInfoResponse{};
}

} // namespace storm
