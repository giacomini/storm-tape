#ifndef CANCEL_RESPONSE_HPP
#define CANCEL_RESPONSE_HPP

#include "types.hpp"

namespace storm {

struct CancelResponse
{
  StageId id{};
  LogicalPaths invalid{};

  explicit CancelResponse(StageId i, LogicalPaths v = {})
      : id(std::move(i))
      , invalid(std::move(v))
  {}
};

} // namespace storm

#endif