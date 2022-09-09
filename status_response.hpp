#ifndef STATUS_RESPONSE_HPP
#define STATUS_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {
class Configuration;
class StageRequest;

class StatusResponse
{
 private:
  std::string m_id;
  StageRequest const* m_stage;

 public:
  StatusResponse(std::string const& id, StageRequest const* stage)
      : m_id(std::move(id))
      , m_stage(stage)
  {}
  StatusResponse(StageRequest const* stage)
      : m_stage(stage)
  {}

  crow::response status(boost::json::object const& jbody) const;
  std::string const& id() const;
  StageRequest const* stage() const;

  static crow::response bad_request();
  static crow::response not_found();
};

} // namespace storm

#endif