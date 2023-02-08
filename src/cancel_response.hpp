#ifndef CANCEL_RESPONSE_HPP
#define CANCEL_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <optional>
#include <string>

#include "stage_request.hpp"

namespace storm {
class Configuration;

class CancelResponse
{
  StageId m_id;
  Paths m_invalid;

 public:
  CancelResponse(StageId id = {}, Paths invalid = {})
      : m_id(std::move(id))
      , m_invalid(std::move(invalid))
  {}

  StageId const& id() const;
  Paths const& invalid() const;
  static crow::response bad_request_with_body(boost::json::object jbody);
  static crow::response bad_request();
  static crow::response not_found();
  static crow::response cancelled();
};

} // namespace storm

#endif