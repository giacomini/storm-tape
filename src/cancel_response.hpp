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
 private:
  std::string m_id;
  std::optional<StageRequest> const m_stage{std::nullopt};
  std::vector<std::filesystem::path> m_invalid;

 public:
  CancelResponse(std::string id, std::optional<StageRequest> const stage,
                 std::vector<std::filesystem::path> invalid = {})
      : m_id(std::move(id))
      , m_stage(stage)
      , m_invalid(std::move(invalid))
  {}

  std::string const& id() const;
  std::optional<StageRequest> const stage() const;
  std::vector<std::filesystem::path> const& invalid() const;
  static crow::response bad_request_with_body(boost::json::object jbody);
  static crow::response bad_request();
  static crow::response not_found();
  static crow::response cancelled();
};

} // namespace storm

#endif