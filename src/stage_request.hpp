#ifndef STAGE_REQUEST_HPP
#define STAGE_REQUEST_HPP

#include "file.hpp"
#include <chrono>

namespace storm {
struct StageRequest
{
  Files files;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point started_at;
  std::chrono::system_clock::time_point completed_at;

  inline static struct Tag
  {
  } tag{};
  explicit StageRequest(Files f = {})
      : files(std::move(f))
      , created_at{std::chrono::system_clock::now()}
      , started_at{}
      , completed_at{}
  {}
};

} // namespace storm

#endif