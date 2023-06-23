#ifndef STORM_FILE_HPP
#define STORM_FILE_HPP

#include "types.hpp"
#include <string>
#include <vector>

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
  bool on_disk() const {
    return locality == Locality::disk || locality == Locality::disk_and_tape;
  }
};

inline auto is_final(File::State state)
{
  return state == File::State::cancelled || state == File::State::failed
      || state == File::State::completed;
}
std::string to_string(File::State state);

using Files = std::vector<File>;

} // namespace storm

#endif
