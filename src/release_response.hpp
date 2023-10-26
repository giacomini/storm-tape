#ifndef RELEASE_RESPONSE_HPP
#define RELEASE_RESPONSE_HPP

#include "types.hpp"

namespace storm {

struct ReleaseResponse
{
  StageId id{};
  LogicalPaths invalid{};

  ReleaseResponse(StageId i, LogicalPaths v = {})
      : id(std::move(i))
      , invalid(std::move(v))
  {}
};

} // namespace storm

#endif