#ifndef TAPE_SERVICE_HPP
#define TAPE_SERVICE_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace storm {
class Database;
class StageRequest;
class CancelRequest;
class ReleaseRequest;
class RequestWithPaths;
class ArchiveInfo;
class StageResponse;
class StatusResponse;
class CancelResponse;
class DeleteResponse;
class ReleaseResponse;
class ArchiveResponse;
class File;

class TapeService
{
  Database* m_db;
  std::vector<std::string> m_id_buffer;

 public:
  TapeService(Database& db)
      : m_db(&db)
  {}

  StageResponse stage(StageRequest const& stage_request);
  StatusResponse status(std::string const& id);
  CancelResponse cancel(std::string const& id, CancelRequest const& cancel);
  DeleteResponse erase(std::string const& id);
  ReleaseResponse release(std::string const& id, ReleaseRequest const& release);
  ArchiveResponse archive(storm::ArchiveInfo const& info);
};

} // namespace storm

#endif