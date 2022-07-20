#include "tape_service.hpp"
#include "database.hpp"

json::object TapeService::stage(StageRequest& stage_request, Database& db)
{
  std::string id;
  id = db.insert(stage_request);
  json::object jbody{{"requestId", id}};
  return jbody;
}