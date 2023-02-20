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
class Storage;
class StageRequest;
class CancelRequest;
class ReleaseRequest;
class RequestWithPaths;
class ArchiveInfoRequest;
class StageResponse;
class StatusResponse;
class CancelResponse;
class DeleteResponse;
class ReleaseResponse;
class ArchiveInfoResponse;
class ReadyTakeOverResponse;
class TakeOverRequest;
class TakeOverResponse;
class File;

class TapeService
{
  boost::uuids::random_generator m_uuid_gen;
  Database* m_db;
  Storage* m_storage;

 public:
  TapeService(Database& db, Storage& storage)
      : m_db(&db), m_storage(&storage)
  {}

  StageResponse stage(StageRequest stage_request);
  StatusResponse status(StageId const& id);
  CancelResponse cancel(StageId const& id, CancelRequest cancel);
  DeleteResponse erase(StageId const& id);
  ReleaseResponse release(StageId const& id, ReleaseRequest release);
  ArchiveInfoResponse archive_info(ArchiveInfoRequest info);

  // for GEMSS
  ReadyTakeOverResponse ready_take_over();
  TakeOverResponse take_over(TakeOverRequest);
};

} // namespace storm

#endif