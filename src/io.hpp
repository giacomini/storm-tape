#ifndef IO_HPP
#define IO_HPP

#include "file.hpp"
#include <boost/json.hpp>
#include <crow.h>
#include <regex>
#include "stage_request.hpp"
#include "requests_with_paths.hpp"

namespace storm {

class StageResponse;
class StatusResponse;
class CancelResponse;
class DeleteResponse;
class ReleaseResponse;
class ArchiveInfoResponse;
class Configuration;

struct HostInfo
{
  std::string proto;
  std::string host;
};

crow::response to_crow_response(StageResponse const& resp,
                                HostInfo const& info);

crow::response to_crow_response(StatusResponse const& resp);

boost::json::object file_missing_to_json(Paths const& missing,
                                         std::string const& id);
crow::response to_crow_response(CancelResponse const& resp);
crow::response to_crow_response(ReleaseResponse const& resp);

boost::json::array not_in_archive_to_json(Paths const& missing,
                                          boost::json::array& jbody);
boost::json::array archive_to_json(Files const& file,
                                   boost::json::array& jbody);
crow::response to_crow_response(ArchiveInfoResponse const& resp);

Files from_json(std::string_view const& body, StageRequest::Tag);
Paths from_json(std::string_view const& body, RequestWithPaths::Tag);

HostInfo get_host(crow::request const& req, Configuration const& conf);

template<class Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
  return static_cast<std::underlying_type_t<Enum>>(e);
}

} // namespace storm

#endif