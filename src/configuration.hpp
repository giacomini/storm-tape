#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include "types.hpp"
#include <iosfwd>
#include <string>
#include <vector>

namespace storm {

struct StorageArea
{
  std::string name;
  PhysicalPath root;
  LogicalPaths access_points;
};

using StorageAreas = std::vector<StorageArea>;
// log levels match Crow's
// 0 - Debug, 1 - Info, 2 - Warning, 3 - Error, 4 - Critical
using LogLevel = int;
struct Configuration
{
  std::string hostname = "localhost";
  std::uint16_t port   = 8080;
  StorageAreas storage_areas;
  LogLevel log_level = 1;
  bool mirror_mode = false;
};

Configuration load_configuration(std::istream& is);
Configuration load_configuration(Path const& p);

} // namespace storm

#endif