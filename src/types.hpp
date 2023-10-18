#ifndef STORM_TYPES_HPP
#define STORM_TYPES_HPP

#include <filesystem>
#include <string>
#include <vector>
namespace storm {

namespace fs = std::filesystem;

struct LogicalPath;
struct PhysicalPath;
using Path          = fs::path;
using Paths         = std::vector<Path>;
using LogicalPaths  = std::vector<LogicalPath>;
using PhysicalPaths = std::vector<PhysicalPath>;
using TimePoint     = long long int;
using StageId       = std::string;

enum class Locality : unsigned char
{
  unavailable, // this is zero to optimize the initialization of Localities
  disk,
  tape,
  disk_and_tape,
  lost,
  none
};

struct LogicalPath : fs::path
{
  using fs::path::path;
};

struct PhysicalPath : fs::path
{
  using fs::path::path;
};

std::string to_string(Locality locality);

} // namespace storm

#endif
