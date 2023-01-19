#ifndef STAGE_RESPONSE_HPP
#define STAGE_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {
class Configuration;

class StageResponse
{
 private:
  std::string m_id;
  std::vector<std::filesystem::path> m_invalid;

 public:
  StageResponse(std::string const& id,
                std::vector<std::filesystem::path>& invalid)
      : m_id(std::move(id))
      , m_invalid(std::move(invalid))
  {}
  StageResponse(std::string const& id)
      : m_id(std::move(id))
  {}
  StageResponse()
  {}

  crow::response staged(boost::json::object const& jbody,
                        std::map<std::string, std::string> const& map) const;
  static crow::response bad_request();

  std::string const& id() const;
};

} // namespace storm

#endif