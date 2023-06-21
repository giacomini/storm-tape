#ifndef STAGE_REQUEST_HPP
#define STAGE_REQUEST_HPP

#include "file.hpp"

namespace storm {
struct StageRequest
{
  Files files;
  TimePoint created_at{};
  TimePoint started_at{};
  TimePoint completed_at{};

  struct Tag {};
  static constexpr Tag tag{};
  bool update_timestamps();
};

} // namespace storm

#endif