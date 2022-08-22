#ifndef TAPE_SERVICE_HPP
#define TAPE_SERVICE_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace storm {
class Database;
class StageRequest;
class TapeResponse;
class Cancel;
class Release;
class RequestWithPaths;
class Archiveinfo;
class File;

class TapeService
{
  Database* m_db;
  std::vector<std::string> m_id_buffer;

 public:
  TapeService(Database& db)
      : m_db(&db)
  {}

  TapeResponse stage(StageRequest& stage_request);
  TapeResponse cancel(Cancel cancel, StageRequest* stage_request);
  TapeResponse erase(std::string const& id);
  TapeResponse release(Release release, StageRequest* stage_request);
  std::vector<std::filesystem::path> archive();

  TapeResponse check_invalid(RequestWithPaths cancel,
                             std::vector<std::filesystem::path> both,
                             std::string const& id = "");
  std::vector<std::filesystem::path>
  staged_to_cancel(Cancel cancel, StageRequest* stage_request);
  std::vector<std::filesystem::path>
  staged_to_release(Release release, StageRequest* stage_request);
  std::vector<std::filesystem::path>
  info_from_archive(Archiveinfo info,
                    std::vector<std::filesystem::path> archive);
  std::vector<File>
  compute_remaining(Archiveinfo info,
                    std::vector<std::filesystem::path> missing);
  StageRequest const* find(std::string id);
  StageRequest* find_and_edit(std::string id);
};

} // namespace storm

#endif