#ifndef REQUEST_WITH_PATHS_HPP
#define REQUEST_WITH_PATHS_HPP

#include "types.hpp"

namespace storm {

struct RequestWithPaths
{
  struct Tag {};
  static constexpr Tag tag{};

  Paths paths;
};

struct CancelRequest : RequestWithPaths
{
};

struct ReleaseRequest : RequestWithPaths
{
};

struct ArchiveInfoRequest : RequestWithPaths
{
};

} // namespace storm

#endif