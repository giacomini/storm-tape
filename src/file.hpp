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
  Path logical_path{};
  Path physical_path{};
  State state{State::submitted};
  Locality locality{Locality::unavailable};
  TimePoint started_at{0};
  TimePoint finished_at{0};
};

std::string to_string(File::State state);

using Files = std::vector<File>;

} // namespace storm

#endif