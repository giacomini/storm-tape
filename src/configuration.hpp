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
  LogicalPath access_point;
};

using StorageAreas = std::vector<StorageArea>;

struct Configuration
{
  std::string hostname = "localhost";
  std::uint16_t port   = 8080;
  StorageAreas storage_areas;
};

Configuration load_configuration(std::istream& is);
Configuration load_configuration(std::filesystem::path const& p);

} // namespace storm

#endif