#ifndef STORM_FILE_HPP
#define STORM_FILE_HPP

#include "types.hpp"
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
  enum class Locality : unsigned char
  {
    unknown,
    on_tape = 1,
    on_disk = 2
  };

  Path path;
  State state{State::submitted};
  Locality locality{Locality::on_tape};
};
} // namespace storm

#endif