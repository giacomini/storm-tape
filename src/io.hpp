#ifndef IO_HPP
#define IO_HPP

#include "errors.hpp"
#include "file.hpp"
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "takeover_request.hpp"
#include <boost/json.hpp>

namespace crow {
class request;
class response;
} // namespace crow

namespace storm {

class StageResponse;
class StatusResponse;
class CancelResponse;
class DeleteResponse;
class ReleaseResponse;
class ArchiveInfoResponse;
class ReadyTakeOverResponse;
class TakeOverResponse;
class InProgressResponse;
class Configuration;

struct HostInfo
{
  std::string proto;
  std::string host;
  std::string port;
};

crow::response to_crow_response(StageResponse const& resp,
                                HostInfo const& info);

crow::response to_crow_response(StatusResponse const& resp);
crow::response to_crow_response(DeleteResponse const& resp);
crow::response to_crow_response(CancelResponse const& resp);
crow::response to_crow_response(ReleaseResponse const& resp);
boost::json::array archive_to_json(Files const& file,
                                   boost::json::array& jbody);
crow::response to_crow_response(ArchiveInfoResponse const& resp);

crow::response to_crow_response(ReadyTakeOverResponse const& resp);
crow::response to_crow_response(TakeOverResponse const& resp);
crow::response to_crow_response(InProgressResponse const& resp);
crow::response to_crow_response(storm::HttpError const& exception);

Files from_json(std::string_view body, StageRequest::Tag);
LogicalPaths from_json(std::string_view body, RequestWithPaths::Tag);

void fill_hostinfo_from_forwarded(HostInfo& info, std::string const& http_forwarded);
HostInfo get_hostinfo(crow::request const& req, Configuration const& conf);

std::size_t from_body_params(std::string_view body, TakeOverRequest::Tag);

} // namespace storm

#endif