#include "file.hpp"

namespace storm {

std::string to_string(File::State state)
{
  using namespace std::string_literals;

  switch (state) {
  case File::State::submitted:
    return "SUBMITTED"s;
  case File::State::started:
    return "STARTED"s;
  case File::State::cancelled:
    return "CANCELLED"s;
  case File::State::failed:
    return "FAILED"s;
  case File::State::completed:
    return "COMPLETED"s;
  default:
    return "UNKNOWN"s;
  }
}

} // namespace storm