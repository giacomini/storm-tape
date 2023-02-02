#ifndef RELEASE_RESPONSE_HPP
#define RELEASE_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {
class Configuration;
class StageRequest;

class ReleaseResponse
{
 private:
  std::string m_id;
  StageRequest const* m_stage{nullptr};
  std::vector<std::filesystem::path> m_invalid;

 public:
  ReleaseResponse() = default;
  ReleaseResponse(StageRequest const* stage)
      : m_stage(stage)
  {}
  ReleaseResponse(std::string id,
                  std::vector<std::filesystem::path> invalid)
      : m_id(std::move(id))
      , m_invalid(std::move(invalid))
  {}

  std::string const& id() const;
  StageRequest const* stage() const;
  std::vector<std::filesystem::path> const& invalid() const;
  static crow::response bad_request_with_body(boost::json::object jbody);
  static crow::response bad_request();
  static crow::response not_found();
  static crow::response released();
};

} // namespace storm

#endif