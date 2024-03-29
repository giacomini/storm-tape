#include "io.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "configuration.hpp"
#include "delete_response.hpp"
#include "errors.hpp"
#include "in_progress_response.hpp"
#include "readytakeover_response.hpp"
#include "release_response.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include "types.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/url/parse.hpp>
#include <boost/variant2.hpp>
#include <crow.h>
#include <fmt/core.h>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <regex>
#include <sstream>

namespace storm {

boost::json::object to_json(StageResponse const& resp)
{
  return boost::json::object{{"requestId", resp.id()}};
}

static auto make_location(HostInfo const& info, StageId const& id)
{
  return fmt::format("{}://{}:{}/api/v1/stage/{}", info.proto, info.host,
                     info.port, id);
}

crow::response to_crow_response(StageResponse const& resp, HostInfo const& info)
{
  if (resp.id().empty()) {
    return crow::response{500};
  } else {
    auto jbody = to_json(resp);
    crow::response cresp{crow::status::CREATED, "json",
                         fmt::format("{}\n", boost::json::serialize(jbody))};
    cresp.set_header("Location", make_location(info, resp.id()));
    return cresp;
  }
}

crow::response to_crow_response(StatusResponse const& resp)
{
  auto const& stage   = resp.stage();
  auto const& id      = resp.id();
  auto const& m_files = stage.files;

  boost::json::array files;
  files.reserve(m_files.size());
  std::transform( //
      m_files.begin(), m_files.end(), std::back_inserter(files),
      [](File const& file) {
        return boost::json::object{{"path", file.logical_path.c_str()},
                                   {"state", to_string(file.state)}};
      });
  boost::json::object jbody;
  jbody["id"]          = id;
  jbody["createdAt"]   = stage.created_at;
  jbody["startedAt"]   = stage.started_at;
  jbody["completedAt"] = stage.completed_at;
  jbody["files"]       = files;

  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

// Creates a JSON object when one or more files targeted for cancellation do
// not belong to the initially submitted stage request.
boost::json::object file_missing_to_json(LogicalPaths const& missing,
                                         StageId const& id)
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

crow::response to_crow_response(DeleteResponse const&) //-V835 for uniformity
{
  return crow::response{crow::status::OK};
}

crow::response to_crow_response(CancelResponse const& resp)
{
  auto jbody = file_missing_to_json(resp.invalid, resp.id);
  return {crow::status::BAD_REQUEST, "json",
          fmt::format("{}\n", boost::json::serialize(jbody))};
}

crow::response to_crow_response(ReleaseResponse const& resp)
{
  auto jbody = file_missing_to_json(resp.invalid, resp.id);
  return {crow::status::BAD_REQUEST, "json",
          fmt::format("{}\n", boost::json::serialize(jbody))};
}

struct PathInfoVisitor
{
  std::string logical_path;
  auto operator()(Locality locality) const
  {
    return boost::json::object{{"path", logical_path},
                               {"locality", to_string(locality)}};
  }
  auto operator()(std::string const& msg) const
  {
    return boost::json::object{{"path", logical_path}, {"error", msg}};
  }
};

crow::response to_crow_response(ArchiveInfoResponse const& resp)
{
  boost::json::array jbody;
  auto& infos = resp.infos;
  jbody.reserve(infos.size());

  std::transform(infos.begin(), infos.end(), std::back_inserter(jbody),
                 [&](PathInfo const& info) {
                   PathInfoVisitor visitor{info.path.string()};
                   return boost::variant2::visit(visitor, info.info);
                 });

  return crow::response{crow::status::OK, "json",
                        fmt::format("{}\n", boost::json::serialize(jbody))};
}

crow::response to_crow_response(ReadyTakeOverResponse const& resp) //-V835 for uniformity
{
  return crow::response{crow::status::OK, "txt",
                        fmt::format("{}\n", resp.n_ready)};
}

crow::response to_crow_response(TakeOverResponse const& resp)
{
  auto const body =
      std::accumulate(resp.paths.begin(), resp.paths.end(), std::string{}, //
                      [](std::string const& acc, PhysicalPath const& path) {
                        return fmt::format("{}unused {}\n", acc, path.string());
                      });
  return crow::response{crow::status::OK, "txt", body};
}

crow::response to_crow_response(InProgressResponse const& resp)
{
  auto const body =
      std::accumulate(resp.paths.begin(), resp.paths.end(), std::string{}, //
                      [](std::string const& acc, PhysicalPath const& path) {
                        return fmt::format("{}{}\n", acc, path.string());
                      });
  return crow::response{crow::status::OK, "txt", body};
}

crow::response to_crow_response(storm::HttpError const& e)
{
  static auto constexpr body_format = R"({{"status":{},"title":"{}"}})";
  auto const body = fmt::format(body_format, e.status_code(), e.what());
  auto response   = crow::response{e.status_code(), body};
  response.set_header("Content-Type", "application/problem+json");
  return response;
}

Files from_json(std::string_view body, StageRequest::Tag)
{
  try {
    auto const value =
        boost::json::parse(boost::json::string_view{body.data(), body.size()});
    auto& jfiles = value.as_object().at("files").as_array();
    Files files;
    files.reserve(jfiles.size());

    std::transform(                   //
        jfiles.begin(), jfiles.end(), //
        std::back_inserter(files),    //
        [](auto& jfile) {
          std::string_view sv = jfile.as_object().at("path").as_string();
          return File{LogicalPath{Path{sv}.lexically_normal()}};
        } //
    );
    return files;
  } catch (boost::exception const&) {
    throw BadRequest("Invalid JSON");
  }
}

LogicalPaths from_json(std::string_view body, RequestWithPaths::Tag)
{
  try {
    LogicalPaths paths;
    auto const value =
        boost::json::parse(boost::json::string_view{body.data(), body.size()});

    auto const& o = value.as_object();

    if (auto const* p = o.if_contains("paths"); p != nullptr) {
      auto const& ja = p->as_array();
      paths.reserve(ja.size());

      std::transform(                //
          ja.begin(), ja.end(),      //
          std::back_inserter(paths), //
          [](auto& jpath) {
            std::string_view sv = jpath.as_string();
            return LogicalPath{Path{sv}.lexically_normal()};
          } //
      );

    } else {
      auto& ja = o.at("files").as_array();
      paths.reserve(ja.size());

      std::transform(                //
          ja.begin(), ja.end(),      //
          std::back_inserter(paths), //
          [](auto& jfile) {
            std::string_view sv = jfile.as_object().at("path").as_string();
            return LogicalPath{Path{sv}.lexically_normal()};
          } //
      );
    }

    return paths;

  } catch (boost::exception const&) {
    throw BadRequest("Invalid JSON");
  }
}

void fill_hostinfo_from_forwarded(HostInfo& info,
                                  std::string const& http_forwarded)
{
  static std::regex const proto_re("proto=(http|https)(;|$)");
  static std::regex const host_re("host=([^;]+)(;|$)");
  static std::regex const port_re("port=([0-9]+)(;|$)");

  if (std::smatch match; std::regex_search(
          http_forwarded.begin(), http_forwarded.end(), match, proto_re)) {
    info.proto = match[1];
    if (info.proto == "http") {
      info.port = "80";
    } else if (info.proto == "https") {
      info.port = "443";
    }
  }

  if (std::smatch match; std::regex_search(
          http_forwarded.begin(), http_forwarded.end(), match, host_re)) {
    info.host = match[1];
  }

  if (std::smatch match; std::regex_search(
          http_forwarded.begin(), http_forwarded.end(), match, port_re)) {
    auto const port = boost::lexical_cast<int>(match[1]);
    if (port > 0 && port < 65'536) {
      info.port = match[1];
    }
  }
}

void fill_hostinfo_from_host(HostInfo& info, std::string const& http_host)
{
  // the value of the Host field is of the form hostname[:port]
  auto const pos = http_host.find(':');
  if (pos != 0) {
    info.host = http_host.substr(0, pos);
  }
  if (pos != std::string::npos) {
    auto port_s = http_host.substr(pos + 1);
    int port; // uninitialized
    if (boost::conversion::try_lexical_convert(port_s, port)) {
      if (port > 0 && port < 65536) {
        info.port = std::move(port_s);
      }
    }
  }
}

HostInfo get_hostinfo(crow::request const& req, Configuration const& conf)
{
  HostInfo result{"http", conf.hostname, std::to_string(conf.port)};

  if (auto const http_forwarded = req.get_header_value("Forwarded");
      !http_forwarded.empty()) {
    fill_hostinfo_from_forwarded(result, http_forwarded);
  } else if (auto const http_host = req.get_header_value("Host");
             !http_host.empty()) {
    fill_hostinfo_from_host(result, http_host);
  }

  return result;
}

std::size_t from_body_params(std::string_view body, TakeOverRequest::Tag)
{
  std::size_t n_files{1};
  if (!body.empty()) {
    auto const dummy_url = std::string("/a?").append(body);
    auto url_view        = boost::urls::parse_origin_form(dummy_url);
    if (!url_view.has_value()) {
      throw BadRequest("Invalid query parameters");
    }
    auto params = url_view.value().params();
    auto it     = params.find("first");
    if (it != params.end()) {
      auto p         = *it;
      auto& v        = p.value;
      auto [ptr, ec] = std::from_chars(std::to_address(v.begin()),
                                       std::to_address(v.end()), n_files);
      if (ptr != std::to_address(v.end())
          || ec != std::errc{}) { // not all input has been consumed or an error
                                  // occoured
        throw BadRequest("Invalid query parameters");
      }
    }
  }
  return n_files;
}

InProgressRequest from_query_params(crow::query_string const& qs,
                                    InProgressRequest::Tag)
{
  InProgressRequest result{};

  if (auto v = qs.get("n")) {
    int n;
    if (boost::conversion::try_lexical_convert(std::string{v}, n) && n > 0) {
      result.n_files = static_cast<std::size_t>(n);
    }
  }
  if (auto v = qs.get("precise")) {
    int precise;
    if (boost::conversion::try_lexical_convert(std::string{v}, precise) && precise >= 0) {
      result.precise = precise;
    }
  }

  return result;
}

} // namespace storm