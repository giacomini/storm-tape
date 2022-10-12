#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP
#include <string>

namespace storm {
constexpr auto c_well_known_uri{"/.well-known/wlcg-tape-rest-api"};
struct Configuration
{
  std::string hostname = "localhost";
  std::string base_url = "https://" + hostname + ":" + std::to_string(port);
  std::string api_uri  = base_url + "/api/v1";
  std::string const well_known_uri = c_well_known_uri;
  std::uint16_t port         = 8080;
};
} // namespace storm

#endif