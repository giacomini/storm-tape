#ifndef STORM_TYPES_HPP
#define STORM_TYPES_HPP

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>
namespace storm {

namespace fs = std::filesystem;

using Path      = fs::path;
using Paths     = std::vector<Path>;
using TimePoint = long long int;
using StageId   = std::string;
using Clock     = std::chrono::system_clock;

enum class Locality : unsigned char
{
  unavailable, // this is zero to optimize the initialization of Localities
  disk,
  tape,
  disk_and_tape,
  lost,
  none
};

std::string to_string(Locality locality);

} // namespace storm

#endif
