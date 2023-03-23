#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include "types.hpp"
#include <string>
#include <vector>
#include <iosfwd>
namespace storm {

struct StorageArea
{
  std::string name;
  Path root;
  Path access_point;
};

using StorageAreas = std::vector<StorageArea>;

struct Configuration
{
  std::string hostname = "localhost";
  std::uint16_t port   = 8080;
  StorageAreas storage_areas;
};

Configuration load_configuration(std::istream& is);
Configuration load_configuration(Path const& p);

} // namespace storm

#endif