#ifndef STORM_FILE_HPP
#define STORM_FILE_HPP

#include "types.hpp"
#include <vector>
#include <string>
namespace storm {
struct File
{
  enum class State : unsigned char
  {
    submitted,
    started,
    cancelled,
    failed,
    completed
  };
  Path path;
  State state{State::submitted};
  Locality locality{Locality::unavailable};
};

std::string to_string(File::State state);

using Files = std::vector<File>;

} // namespace storm

#endif