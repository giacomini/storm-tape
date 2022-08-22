#include "tape_service.hpp"
#include "database.hpp"
#include "requests_with_paths.hpp"
#include "tape_response.hpp"

storm::TapeResponse
storm::TapeService::stage(storm::StageRequest& stage_request)
{
  // std::string id;
  std::string const& id = m_db->insert(stage_request);
  TapeResponse resp(id);
  return resp;
}

storm::TapeResponse
storm::TapeService::cancel(storm::Cancel cancel,
                           storm::StageRequest* stage_request)
{
  auto m_files = stage_request->getFiles();
  for (storm::File pth : cancel.paths) {
    int idx = 0;
    for (storm::File file : m_files) {
      if (strcmp(pth.path.c_str(), file.path.c_str()) == 0) {
        file.state   = storm::File::State::cancelled;
        m_files[idx] = file;
      };
      idx++;
    }
  }
  stage_request->setFiles(m_files);
  TapeResponse resp;
  return resp;
}

storm::TapeResponse storm::TapeService::erase(std::string const& id)
{
  auto const c = m_db->erase(id);
  assert(c == 1);
  m_id_buffer.clear();
  TapeResponse resp{id};
  return resp;
}

storm::TapeResponse
storm::TapeService::release(storm::Release release,
                            storm::StageRequest* stage_request)
{
  TapeResponse dummy;
  return dummy;
  //......................................
  // What does "release all files" means???
  //......................................
}

std::vector<std::filesystem::path> storm::TapeService::archive()
{
  std::vector<std::filesystem::path> file_buffer;
  std::vector<std::string> id_buffer =
      dynamic_cast<storm::MockDatabase*>(m_db)->storm::MockDatabase::id_buffer;
  m_id_buffer = id_buffer;
  file_buffer.reserve(m_id_buffer.size());
  for (auto id : m_id_buffer) {
    std::cout << id << std::endl;
    storm::StageRequest const* stage = m_db->find(id);
    for (auto& file : stage->getFiles()) {
      file_buffer.push_back(file.path);
    }
  }
  return file_buffer;
}

storm::TapeResponse
storm::TapeService::check_invalid(storm::RequestWithPaths cancel,
                                  std::vector<std::filesystem::path> both,
                                  std::string const& id)
{
  auto proj =
      [](storm::File const& stage_file) -> std::filesystem::path const& {
    return stage_file.path;
  };

  std::vector<std::filesystem::path> invalid;
  assert(cancel.paths.size() > both.size());
  invalid.reserve(cancel.paths.size() - both.size());
  std::set_difference(
      boost::make_transform_iterator(cancel.paths.begin(), proj),
      boost::make_transform_iterator(cancel.paths.end(), proj), both.begin(),
      both.end(), std::back_inserter(invalid));
  TapeResponse resp{id, invalid};
  return resp;
}

std::vector<std::filesystem::path>
storm::TapeService::staged_to_cancel(storm::Cancel cancel,
                                     storm::StageRequest* stage)
{
  auto proj =
      [](storm::File const& stage_file) -> std::filesystem::path const& {
    return stage_file.path;
  };

  std::vector<std::filesystem::path> both;
  both.reserve(cancel.paths.size());
  std::set_intersection(
      boost::make_transform_iterator(cancel.paths.begin(), proj),
      boost::make_transform_iterator(cancel.paths.end(), proj),
      boost::make_transform_iterator(stage->getFiles().begin(), proj),
      boost::make_transform_iterator(stage->getFiles().end(), proj),
      std::back_inserter(both));
  return both;
}

std::vector<std::filesystem::path>
storm::TapeService::staged_to_release(storm::Release release,
                                      storm::StageRequest* stage)
{
  auto proj =
      [](storm::File const& stage_file) -> std::filesystem::path const& {
    return stage_file.path;
  };

  std::vector<std::filesystem::path> both;
  both.reserve(release.paths.size());
  std::set_intersection(
      boost::make_transform_iterator(release.paths.begin(), proj),
      boost::make_transform_iterator(release.paths.end(), proj),
      boost::make_transform_iterator(stage->getFiles().begin(), proj),
      boost::make_transform_iterator(stage->getFiles().end(), proj),
      std::back_inserter(both));
  return both;
}

std::vector<std::filesystem::path> storm::TapeService::info_from_archive(
    storm::Archiveinfo info, std::vector<std::filesystem::path> archive)
{
  auto proj =
      [](storm::File const& stage_file) -> std::filesystem::path const& {
    return stage_file.path;
  };

  std::vector<std::filesystem::path> both;
  both.reserve(info.paths.size());
  std::set_intersection(
      boost::make_transform_iterator(info.paths.begin(), proj),
      boost::make_transform_iterator(info.paths.end(), proj), archive.begin(),
      archive.end(), std::back_inserter(both));
  return both;
}

std::vector<storm::File> storm::TapeService::compute_remaining(
    storm::Archiveinfo info, std::vector<std::filesystem::path> missing)
{
  std::vector<storm::File> remaining;
  for (auto i : info.paths) {
    if (std::find(missing.begin(), missing.end(), i.path) != missing.end())
      continue;
    remaining.push_back(i);
  }
  return remaining;
}

storm::StageRequest const* storm::TapeService::find(std::string id)
{
  storm::StageRequest const* stage = m_db->find(id);
  return stage;
}

storm::StageRequest* storm::TapeService::find_and_edit(std::string id)
{
  storm::StageRequest* stage = m_db->find(id);
  return stage;
}