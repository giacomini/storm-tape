#ifndef RELEASE_RESPONSE_HPP
#define RELEASE_RESPONSE_HPP

#include "types.hpp"
#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {

class StageRequest;

class ReleaseResponse
{
 private:
  StageId m_id;
  StageRequest const* m_stage{nullptr};
  Paths m_invalid;

 public:
  ReleaseResponse() = default;
  ReleaseResponse(StageRequest const* stage)
      : m_stage(stage)
  {}
  ReleaseResponse(StageId id, Paths invalid)
      : m_id(std::move(id))
      , m_invalid(std::move(invalid))
  {}

  StageId const& id() const;
  StageRequest const* stage() const;
  Paths const& invalid() const;
  static crow::response bad_request_with_body(boost::json::object jbody);
  static crow::response bad_request();
  static crow::response not_found();
  static crow::response released();
};

} // namespace storm

#endif