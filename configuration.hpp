#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP
#include <crow.h>
#include <netdb.h>
#include <string>
struct Configuration
{
  std::string hostname = "localhost";
  std::string base_url = "https://" + hostname + ":" + std::to_string(port);
  std::string api_uri  = base_url + "/api/v1";
  std::string well_known_uri = "/.well-known/wlcg-tape-rest-api";
  std::uint16_t port         = 8080;
};

#endif