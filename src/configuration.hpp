#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <string>
#include <fmt/core.h>

namespace storm {

struct Configuration
{
  std::string hostname = "localhost";
  std::string base_url = fmt::format("https://{}:{}", hostname, port);
  std::string api_uri  = fmt::format("{}/api/v1", base_url);
  std::uint16_t port               = 8080;
};

} // namespace storm

#endif