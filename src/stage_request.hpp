#ifndef STAGE_REQUEST_HPP
#define STAGE_REQUEST_HPP

#include "file.hpp"
#include <chrono>

namespace storm {
struct StageRequest
{
  Files files;
  Clock::time_point created_at{Clock::now()};
  Clock::time_point started_at{};
  Clock::time_point completed_at{};

  struct Tag {};
  static constexpr Tag tag{};

  explicit StageRequest(Files f = {})
      : files(std::move(f))
  {}
};

} // namespace storm

#endif