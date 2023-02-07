#ifndef STATUS_RESPONSE_HPP
#define STATUS_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>
#include <optional>

#include "stage_request.hpp"

namespace storm {
class Configuration;

class StatusResponse
{
 private:
  std::string m_id;
  std::optional<StageRequest> const m_stage{std::nullopt};

 public:
  StatusResponse() = default;
  StatusResponse(std::string id, std::optional<StageRequest> const stage)
      : m_id(std::move(id))
      , m_stage(std::move(stage))
  {}

  crow::response status(boost::json::object const& jbody) const;
  std::string const& id() const;
  std::optional<StageRequest> const stage() const;

  static crow::response bad_request();
  static crow::response not_found();
};

} // namespace storm

#endif