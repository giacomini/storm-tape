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
  RequestWithPaths(std::string_view body)
  {
    paths = files_from_json_paths(body);
  }
};

struct Cancel : RequestWithPaths
{
  using RequestWithPaths::RequestWithPaths;
};

struct Release : RequestWithPaths
{
  using RequestWithPaths::RequestWithPaths;
};

struct Archiveinfo : RequestWithPaths
{
  using RequestWithPaths::RequestWithPaths;
};
} // namespace storm

#endif