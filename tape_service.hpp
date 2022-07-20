#ifndef TAPE_SERVICE_HPP
#define TAPE_SERVICE_HPP

#include <boost/json.hpp>
#include <string>

namespace json = boost::json;

class Database;
class StageRequest;

class TapeService
{
 public:
  static json::object stage(StageRequest& stage_request, Database& db);
};

#endif