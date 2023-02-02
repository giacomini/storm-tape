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

StageResponse
TapeService::stage(StageRequest const& stage_request)
{
  // std::string id;
  std::string const& id = m_db->insert(stage_request);
  StageResponse resp{id};
  return resp;
}

StatusResponse TapeService::status(std::string const& id)
{
  StageRequest const* stage = m_db->find(id);
  if (stage == nullptr) {
    StatusResponse resp{stage};
    return resp;
  }
  StatusResponse resp{id, stage};
  return resp;
}

CancelResponse
TapeService::cancel(std::string const& id,
                           CancelRequest const& cancel)
{
  StageRequest* stage = m_db->find(id);
  if (stage == nullptr) {
    CancelResponse resp{stage};
    return resp;
  }

  auto proj =
      [](File const& stage_file) -> std::filesystem::path const& {
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

    CancelResponse resp{id, invalid};
    return resp;
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
    CancelResponse resp{};
    return resp;
  }
}

DeleteResponse TapeService::erase(std::string const& id)
{
  StageRequest const* stage = m_db->find(id);
  if (stage == nullptr) {
    DeleteResponse resp{stage};
    return resp;
  }
  auto const c = m_db->erase(id);
  assert(c == 1);
  m_id_buffer.clear();
  DeleteResponse resp{};
  return resp;
}

ReleaseResponse
TapeService::release(std::string const& id,
                            ReleaseRequest const& release)
{
  StageRequest* stage = m_db->find(id);
  if (stage == nullptr) {
    ReleaseResponse resp{stage};
    return resp;
  }

  auto proj =
      [](File const& stage_file) -> std::filesystem::path const& {
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
    ReleaseResponse resp{id, invalid};
    return resp;
  } else {
    // .......DO SOMETHING?.......
    ReleaseResponse resp{};
    return resp;
  }
}

ArchiveResponse
TapeService::archive(ArchiveInfo const& info)
{
  boost::json::array jbody;
  std::vector<std::filesystem::path> file_buffer;
  m_id_buffer =
      static_cast<MockDatabase*>(m_db)->MockDatabase::m_id_buffer;
  // m_id_buffer = id_buffer;
  file_buffer.reserve(m_id_buffer.size());

  for (auto& id : m_id_buffer) {
    StageRequest const* stage = m_db->find(id);
    for (auto& file : stage->files()) {
      file_buffer.push_back(file.path);
    }
  }

  jbody.reserve(info.paths.size());

  auto proj =
      [](File const& stage_file) -> std::filesystem::path const& {
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

    ArchiveResponse resp{jbody, invalid, remaining};
    return resp;
  } else {
    jbody = archive_to_json(info.paths, jbody);

    ArchiveResponse resp{jbody, info.paths};
    return resp;
  }
}

} // namespace storm
