#ifndef STORM_TYPES_HPP
#define STORM_TYPES_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace storm {

namespace fs = std::filesystem;

using Path      = fs::path;
using Paths     = std::vector<Path>;
using TimePoint = long long int;
using StageId   = std::string;

} // namespace storm

#endif
