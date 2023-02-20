#include "types.hpp"

namespace storm {

std::string to_string(Locality locality)
{
  using namespace std::string_literals;
  switch (locality) {
  case Locality::disk:
    return "DISK"s;
  case Locality::tape:
    return "TAPE"s;
  case Locality::disk_and_tape:
    return "DISK_AND_TAPE"s;
  case Locality::lost:
    return "LOST"s;
  case Locality::none:
    return "NONE"s;
  case Locality::unavailable:
  default:
    return "UNAVAILABLE";
  }
}

} // namespace storm
