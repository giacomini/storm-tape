#ifndef STAGE_REQUEST_HPP
#define STAGE_REQUEST_HPP

#include "file.hpp"
#include <chrono>

namespace storm {
struct StageRequest
{
  Files files;
  TimePoint created_at{};
  TimePoint started_at{};
  TimePoint completed_at{};

  struct Tag {};
  static constexpr Tag tag{};

  explicit StageRequest(Files f = {})
      : files(std::move(f))
  {}
};

} // namespace storm

#endif