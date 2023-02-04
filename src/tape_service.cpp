#include "tape_service.hpp"
#include "archive_response.hpp"
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

StatusResponse TapeService::status(std::string const& id)
{
  auto stage = m_db->find(id);
  return stage == nullptr ? StatusResponse{} : StatusResponse{id, stage};
}

CancelResponse TapeService::cancel(std::string const& id,
                                   CancelRequest const& cancel)
{
  auto stage = m_db->find(id);
  if (stage == nullptr) {
    return CancelResponse{};
  }

  auto proj = [](File const& stage_file) -> std::filesystem::path const& {
    return stage_file.path;
  };

  std::vector<std::filesystem::path> both;
  both.reserve(cancel.paths.size());
  std::set_intersection(
      boost::make_transform_iterator(cancel.paths.begin(), proj),
      boost::make_transform_iterator(cancel.paths.end(), proj),
      boost::make_transform_iterator(stage->files().begin(), proj),
      boost::make_transform_iterator(stage->files().end(), proj),
      std::back_inserter(both));

  if (cancel.paths.size() != both.size()) {
    std::vector<std::filesystem::path> invalid;
    assert(cancel.paths.size() > both.size());
    invalid.reserve(cancel.paths.size() - both.size());
    std::set_difference(
        boost::make_transform_iterator(cancel.paths.begin(), proj),
        boost::make_transform_iterator(cancel.paths.end(), proj), both.begin(),
        both.end(), std::back_inserter(invalid));

    return CancelResponse{id, invalid};
  } else {
    auto m_files = stage->files();
    for (File const& pth : cancel.paths) {
      int idx = 0;
      for (File& file : m_files) {
        if (pth.path == file.path) {
          file.state   = File::State::cancelled;
          m_files[idx] = file;
        };
        idx++;
      }
    }
    stage->files() = m_files;
    return CancelResponse{};
  }
}

DeleteResponse TapeService::erase(std::string const& id)
{
  StageRequest const* stage = m_db->find(id);
  if (stage == nullptr) {
    return DeleteResponse{};
  }
  auto const c = m_db->erase(id);
  assert(c == 1);
  m_id_buffer.clear();
  return DeleteResponse{};
}

ReleaseResponse TapeService::release(std::string const& id,
                                     ReleaseRequest const& release)
{
  StageRequest* stage = m_db->find(id);
  if (stage == nullptr) {
    return ReleaseResponse{};
  }

  auto proj = [](File const& stage_file) -> std::filesystem::path const& {
    return stage_file.path;
  };

  std::vector<std::filesystem::path> both;
  both.reserve(release.paths.size());
  std::set_intersection(
      boost::make_transform_iterator(release.paths.begin(), proj),
      boost::make_transform_iterator(release.paths.end(), proj),
      boost::make_transform_iterator(stage->files().begin(), proj),
      boost::make_transform_iterator(stage->files().end(), proj),
      std::back_inserter(both));

  if (release.paths.size() != both.size()) {
    std::vector<std::filesystem::path> invalid;
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

ArchiveResponse TapeService::archive(ArchiveInfo const& info)
{
  boost::json::array jbody;
  std::vector<std::filesystem::path> file_buffer;
  file_buffer.reserve(m_id_buffer.size());

  for (auto& id : m_id_buffer) {
    StageRequest const* stage = m_db->find(id);
    for (auto& file : stage->files()) {
      file_buffer.push_back(file.path);
    }
  }

  jbody.reserve(info.paths.size());

  auto proj = [](File const& stage_file) -> std::filesystem::path const& {
    return stage_file.path;
  };

  std::vector<std::filesystem::path> both;
  both.reserve(info.paths.size());
  std::set_intersection(
      boost::make_transform_iterator(info.paths.begin(), proj),
      boost::make_transform_iterator(info.paths.end(), proj),
      file_buffer.begin(), file_buffer.end(), std::back_inserter(both));

  if (info.paths.size() != both.size()) {
    std::vector<std::filesystem::path> invalid;
    assert(info.paths.size() > both.size());
    invalid.reserve(info.paths.size() - both.size());
    std::set_difference(
        boost::make_transform_iterator(info.paths.begin(), proj),
        boost::make_transform_iterator(info.paths.end(), proj), both.begin(),
        both.end(), std::back_inserter(invalid));

    jbody = not_in_archive_to_json(invalid, jbody);

    std::vector<File> remaining;
    for (auto& file : info.paths) {
      if (std::find(invalid.begin(), invalid.end(), file.path) != invalid.end())
        continue;
      remaining.push_back(file);
    }

    jbody = archive_to_json(remaining, jbody);

    return ArchiveResponse{jbody, invalid, remaining};
  } else {
    jbody = archive_to_json(info.paths, jbody);

    return ArchiveResponse{jbody, info.paths};
  }
}

} // namespace storm
