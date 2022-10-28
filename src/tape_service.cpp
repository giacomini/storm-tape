#include "tape_service.hpp"
#include "archive_response.hpp"
#include "cancel_response.hpp"
#include "database.hpp"
#include "delete_response.hpp"
#include "release_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"

storm::StageResponse
storm::TapeService::stage(storm::StageRequest const& stage_request)
{
  // std::string id;
  std::string const& id = m_db->insert(stage_request);
  storm::StageResponse resp{id};
  return resp;
}

storm::StatusResponse storm::TapeService::status(std::string const& id)
{
  storm::StageRequest const* stage = m_db->find(id);
  if (stage == nullptr) {
    storm::StatusResponse resp{stage};
    return resp;
  }
  storm::StatusResponse resp{id, stage};
  return resp;
}

storm::CancelResponse
storm::TapeService::cancel(std::string const& id,
                           storm::CancelRequest const& cancel)
{
  storm::StageRequest* stage = m_db->find(id);
  if (stage == nullptr) {
    storm::CancelResponse resp{stage};
    return resp;
  }

  auto proj =
      [](storm::File const& stage_file) -> std::filesystem::path const& {
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

    storm::CancelResponse resp{id, invalid};
    return resp;
  } else {
    auto m_files = stage->files();
    for (storm::File const& pth : cancel.paths) {
      int idx = 0;
      for (storm::File& file : m_files) {
        if (pth.path == file.path) {
          file.state   = storm::File::State::cancelled;
          m_files[idx] = file;
        };
        idx++;
      }
    }
    stage->files() = m_files;
    storm::CancelResponse resp{};
    return resp;
  }
}

storm::DeleteResponse storm::TapeService::erase(std::string const& id)
{
  storm::StageRequest const* stage = m_db->find(id);
  if (stage == nullptr) {
    storm::DeleteResponse resp{stage};
    return resp;
  }
  auto const c = m_db->erase(id);
  assert(c == 1);
  m_id_buffer.clear();
  storm::DeleteResponse resp{};
  return resp;
}

storm::ReleaseResponse
storm::TapeService::release(std::string const& id,
                            storm::ReleaseRequest const& release)
{
  storm::StageRequest* stage = m_db->find(id);
  if (stage == nullptr) {
    storm::ReleaseResponse resp{stage};
    return resp;
  }

  auto proj =
      [](storm::File const& stage_file) -> std::filesystem::path const& {
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
    storm::ReleaseResponse resp{id, invalid};
    return resp;
  } else {
    // .......DO SOMETHING?.......
    storm::ReleaseResponse resp{};
    return resp;
  }
}

storm::ArchiveResponse
storm::TapeService::archive(storm::ArchiveInfo const& info)
{
  boost::json::array jbody;
  std::vector<std::filesystem::path> file_buffer;
  m_id_buffer =
      static_cast<storm::MockDatabase*>(m_db)->storm::MockDatabase::m_id_buffer;
  // m_id_buffer = id_buffer;
  file_buffer.reserve(m_id_buffer.size());

  for (auto& id : m_id_buffer) {
    storm::StageRequest const* stage = m_db->find(id);
    for (auto& file : stage->files()) {
      file_buffer.push_back(file.path);
    }
  }

  jbody.reserve(info.paths.size());

  auto proj =
      [](storm::File const& stage_file) -> std::filesystem::path const& {
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

    jbody = storm::not_in_archive_to_json(invalid, jbody);

    std::vector<storm::File> remaining;
    for (auto& file : info.paths) {
      if (std::find(invalid.begin(), invalid.end(), file.path) != invalid.end())
        continue;
      remaining.push_back(file);
    }

    jbody = storm::archive_to_json(remaining, jbody);

    storm::ArchiveResponse resp{jbody, invalid, remaining};
    return resp;
  } else {
    jbody = storm::archive_to_json(info.paths, jbody);

    storm::ArchiveResponse resp{jbody, info.paths};
    return resp;
  }
}