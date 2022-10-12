#ifndef FILE_HPP
#define FILE_HPP

#include <filesystem>
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
  std::filesystem::path path;
  Locality locality{Locality::on_tape};
  State state{State::submitted};
  void setState(State s){state = s;};
};
} // namespace storm

#endif