#ifndef ARCHIVEINFO_RESPONSE_HPP
#define ARCHIVEINFO_RESPONSE_HPP

#include "types.hpp"
#include <boost/variant2.hpp>
#include <string>

namespace storm {

struct PathInfo
{
  LogicalPath path;
  boost::variant2::variant<Locality, std::string> info;
};

using PathInfos = std::vector<PathInfo>;

struct ArchiveInfoResponse
{
  PathInfos infos;
};

} // namespace storm

#endif