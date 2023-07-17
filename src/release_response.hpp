#ifndef RELEASE_RESPONSE_HPP
#define RELEASE_RESPONSE_HPP

#include "types.hpp"

namespace storm {

struct ReleaseResponse
{
  StageId id{};
  Paths invalid{};

  ReleaseResponse(StageId i, Paths v = {})
      : id(std::move(i))
      , invalid(std::move(v))
  {}
};

} // namespace storm

#endif