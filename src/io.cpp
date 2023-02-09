#include "io.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "configuration.hpp"
#include "delete_response.hpp"
#include "release_response.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include <chrono>
#include <iomanip>
#include <numeric>
#include <sstream>
namespace storm {

boost::json::object to_json(StageResponse const& resp)
{
  return boost::json::object{{"requestId", resp.id()}};
}

crow::response to_crow_response(StageResponse const& sresp,
                                HostInfo const& info)
{
  auto jbody = to_json(sresp);
  // return resp.staged(jbody, info);
  crow::response resp{crow::status::CREATED, "json",
                      boost::json::serialize(jbody)};
  resp.set_header("Location", info.proto + "://" + info.host + "/api/v1/stage/"
                                  + sresp.id());
  return resp;
}

template<class TP>
auto to_seconds(TP time_point)
{
  return std::chrono::duration_cast<std::chrono::seconds>(
             time_point.time_since_epoch())
      .count();
}
crow::response to_crow_response(StatusResponse const& resp)
{
  auto& stage   = resp.stage();
  auto& m_files = stage.files();
  auto& id      = resp.id();

  boost::json::array files;
  files.reserve(m_files.size());
  std::transform( //
      m_files.begin(), m_files.end(), std::back_inserter(files),
      [](File const& file) {
        boost::json::object result{{"path", file.path.c_str()}};
        if (file.locality == File::Locality::on_disk) {
          result.emplace("onDisk", true);
        } else {
          result.emplace("state", to_string(file.state));
        }
        return result;
      });
  boost::json::object jbody;
  jbody["id"]         = id;
  jbody["created_at"] = to_seconds(stage.created_at());
  jbody["started_at"] = to_seconds(stage.started_at());
  jbody["files"]      = files;

  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

// Creates a JSON object when one or more files targeted for cancellation do
// not belong to the initially submitted stage request.
boost::json::object file_missing_to_json(Paths const& missing,
                                         std::string const& id)
{
  std::string const sfile = std::accumulate(
      missing.begin(), missing.end(), std::string{}, [](auto acc, auto s) {
        std::ostringstream os{acc};
        os << std::quoted(s.string(), '\'') << ' ';
        return os.str();
      });

  std::ostringstream message;
  message << "The file" << (missing.size() > 1 ? "s " : " ") << sfile
          << (missing.size() > 1 ? " do " : " does ")
          << "not belong to the STAGE request " << id
          << ". No modification has been made to this request.";

  return boost::json::object{{"title", "File missing from stage request"},
                             {"status", crow::status::BAD_REQUEST},
                             {"detail", message.str()}};
}

crow::response to_crow_response(CancelResponse const& resp)
{
  auto jbody = file_missing_to_json(resp.invalid(), resp.id());
  return crow::response(crow::status::BAD_REQUEST, "json",
                        boost::json::serialize(jbody));
}

crow::response to_crow_response(ReleaseResponse const& resp)
{
  auto jbody = file_missing_to_json(resp.invalid(), resp.id());
  return crow::response(crow::status::BAD_REQUEST, "json",
                        boost::json::serialize(jbody));
}

// Given a JSON array, appends the missing or not accessible files in JSON
// objects, one per file.
boost::json::array not_in_archive_to_json(Paths const& missing,
                                          boost::json::array& jbody)
{
  std::transform( //
      missing.begin(), missing.end(), std::back_inserter(jbody),
      [](Path const& file) {
        boost::json::object result;
        result["error"] = "USER ERROR: file does not exist or is not "
                          "accessible to you";
        result["path"]  = file.c_str();
        return result;
      });
  return jbody;
}

// Given a JSON array, appends the files information in JSON objects, one per
// file.
boost::json::array archive_to_json(std::vector<File> const& files,
                                   boost::json::array& jbody)
{
  std::transform( //
      files.begin(), files.end(), std::back_inserter(jbody),
      [](File const& file) {
        boost::json::object result;
        result["locality"] = to_string(file.locality);
        result["path"]     = file.path.c_str();
        return result;
      });
  return jbody;
}

crow::response to_crow_response(ArchiveInfoResponse const& resp)
{
  return resp.fetched_from_archive(resp.jbody());
}

// Returns a vector of File objects, given a JSON with one or more files listed
// in each "path" field of the "files" array, for a given stage request.
std::vector<File> from_json(std::string_view const& body)
{
  auto const value =
      boost::json::parse(boost::json::string_view{body.data(), body.size()});

  auto& jfiles = value.as_object().at("files").as_array();
  std::vector<File> f_files;
  f_files.reserve(jfiles.size());
  std::transform(jfiles.begin(), jfiles.end(), std::back_inserter(f_files),
                 [](auto& file) {
                   auto status = std::filesystem::status(
                       Path{file.as_object().at("path").as_string().c_str()}
                           .lexically_normal());
                   if (std::filesystem::is_regular_file(status)) {
                     return File{
                         Path{file.as_object().at("path").as_string().c_str()}
                             .lexically_normal(),
                         // /storage/cms/...
                     };
                   } else {
                     return File{
                         Path{file.as_object().at("path").as_string().c_str()}
                             .lexically_normal(),
                         File::State{File::State::failed},
                         // /storage/cms/...
                     };
                   }
                 });
  std::sort(f_files.begin(), f_files.end(),
            [](File const& a, File const& b) { return a.path < b.path; });
  return f_files;
}

// Returns a vector of File objects, given a JSON with one or more files listed
// in the "paths" field, for a given cancel request.
std::vector<File> from_json_paths(std::string_view const& body)
{
  std::vector<File> f_files;
  auto const value =
      boost::json::parse(boost::json::string_view{body.data(), body.size()});

  auto& jfiles = value.as_object().at("paths").as_array();
  f_files.reserve(jfiles.size());
  std::transform(jfiles.begin(), jfiles.end(), std::back_inserter(f_files),
                 [](auto& file) {
                   return File{
                       Path{file.as_string().c_str()}.lexically_normal(),
                   };
                 });
  std::sort(f_files.begin(), f_files.end(),
            [](File const& a, File const& b) { return a.path < b.path; });
  return f_files;
}

HostInfo get_host(crow::request const& req, Configuration const& conf)
{
  HostInfo result{"http", conf.hostname + ':' + std::to_string(conf.port)};

  if (auto const http_forwarded = req.get_header_value("Forwarded");
      !http_forwarded.empty()) {
    std::regex const host_match("host=(.*?)(;|$)");
    std::regex const proto_match("proto=(.*?)(;|$)");
    std::smatch match;
    if (std::regex_search(http_forwarded.begin(), http_forwarded.end(), match, host_match)) {
      result.host = match[1];
    }
    if (std::regex_search(http_forwarded.begin(), http_forwarded.end(), match, proto_match)) {
      result.proto = match[1];
    }
  } else if (auto const http_host = req.get_header_value("Host");
             !http_host.empty()) {
    result.host = http_host;
  }

  return result;
}

} // namespace storm