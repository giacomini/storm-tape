#ifndef REQUEST_WITH_PATHS_HPP
#define REQUEST_WITH_PATHS_HPP

#include "io.hpp"
#include <boost/json.hpp>
#include <filesystem>

namespace storm {
class StageRequest;

struct RequestWithPaths
{
  std::vector<storm::File> paths;
  RequestWithPaths(std::string_view const& body)
  {
    paths = from_json_paths(body);
  }
};

struct CancelRequest : RequestWithPaths
{
  using RequestWithPaths::RequestWithPaths;
};

struct ReleaseRequest : RequestWithPaths
{
  using RequestWithPaths::RequestWithPaths;
};

struct ArchiveInfo : RequestWithPaths
{
  using RequestWithPaths::RequestWithPaths;
};
} // namespace storm

#endif