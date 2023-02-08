#ifndef TAPE_SERVICE_HPP
#define TAPE_SERVICE_HPP

#include "types.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
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
  boost::uuids::random_generator m_uuid_gen;
  Database* m_db;

 public:
  TapeService(Database& db)
      : m_db(&db)
  {}

  StageResponse stage(StageRequest const& stage_request);
  StatusResponse status(StageId const& id);
  CancelResponse cancel(StageId const& id, CancelRequest const& cancel);
  DeleteResponse erase(StageId const& id);
  ReleaseResponse release(StageId const& id, ReleaseRequest const& release);
  ArchiveResponse archive(ArchiveInfo const& info);
};

} // namespace storm

#endif