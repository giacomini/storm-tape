#include "io.hpp"
#include "archive_response.hpp"
#include "cancel_response.hpp"
#include "configuration.hpp"
#include "delete_response.hpp"
#include "release_response.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"

// Creates a JSON object with the given request ID, for a new stage request.
boost::json::object storm::to_json(storm::StageResponse const& resp)
{
  boost::json::object jbody{{"requestId", resp.id()}};
  return jbody;
}

// Creates the CROW response for the stage operation.
crow::response storm::to_crow_response(storm::StageResponse const& resp,
                                       std::map<std::string,std::string> const& map)
{
  auto jbody = to_json(resp);
  return resp.staged(jbody, map);
}

// Creates a JSON object for an already staged request, with a certain
// requestId.
boost::json::object storm::staged_to_json(storm::StageRequest const* stage,
                                          std::string const& id)
{
  boost::json::array files;
  auto m_files = stage->files();
  files.reserve(m_files.size());
  std::transform( //
      m_files.begin(), m_files.end(), std::back_inserter(files),
      [](storm::File const& file) {
        boost::json::object result{{"path", file.path.c_str()}};
        if (file.locality == storm::File::Locality::on_disk) {
          result.emplace("onDisk", true);
        } else {
          result.emplace("state", to_string(file.state));
        }
        return result;
      });
  boost::json::object jbody;
  jbody["id"]         = id;
  jbody["created_at"] = //
      std::chrono::duration_cast<std::chrono::seconds>(
          stage->created_at().time_since_epoch())
          .count();
  jbody["started_at"] = //
      std::chrono::duration_cast<std::chrono::seconds>(
          stage->started_at().time_since_epoch())
          .count();
  jbody["files"] = files;

  return jbody;
}

crow::response storm::to_crow_response(storm::StatusResponse const& resp)
{
  auto jbody = staged_to_json(resp.stage(), resp.id());
  return resp.status(jbody);
}

// Creates a JSON object when one or more files targeted for cancellation do
// not belong to the initially submitted stage request.
boost::json::object
storm::file_missing_to_json(std::vector<std::filesystem::path> const& missing,
                            std::string const& id)
{
  std::string sfile;
  for (auto&& file : missing) {
    sfile += '\'';
    sfile += file;
    sfile += "' ";
  }
  boost::json::object jbody{
      {"title", "File missing from stage request"},
      {"status", 400},
      {"detail", "The files " + sfile + "do not belong to STAGE request " + id
                     + ". No modification has been made to this request."}};
  return jbody;
}

crow::response storm::to_crow_response(storm::CancelResponse const& resp)
{
  auto jbody = storm::file_missing_to_json(resp.invalid(), resp.id());
  return storm::CancelResponse::bad_request_with_body(jbody);
}

crow::response storm::to_crow_response(storm::ReleaseResponse const& resp)
{
  auto jbody = storm::file_missing_to_json(resp.invalid(), resp.id());
  return storm::ReleaseResponse::bad_request_with_body(jbody);
}

// Given a JSON array, appends the missing or not accessible files in JSON
// objects, one per file.
boost::json::array
storm::not_in_archive_to_json(std::vector<std::filesystem::path> const& missing,
                              boost::json::array& jbody)
{
  std::transform( //
      missing.begin(), missing.end(), std::back_inserter(jbody),
      [](std::filesystem::path const& file) {
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
boost::json::array storm::archive_to_json(std::vector<storm::File> const& files,
                                          boost::json::array& jbody)
{
  std::transform( //
      files.begin(), files.end(), std::back_inserter(jbody),
      [](storm::File const& file) {
        boost::json::object result;
        result["locality"] = to_string(file.locality);
        result["path"]     = file.path.c_str();
        return result;
      });
  return jbody;
}

crow::response storm::to_crow_response(storm::ArchiveResponse const& resp)
{
  return resp.fetched_from_archive(resp.jbody());
}

// Returns a vector of File objects, given a JSON with one or more files listed
// in each "path" field of the "files" array, for a given stage request.
std::vector<storm::File> storm::from_json(std::string_view const& body)
{
  auto const value =
      boost::json::parse(boost::json::string_view{body.data(), body.size()});

  auto& jfiles = value.as_object().at("files").as_array();
  std::vector<storm::File> f_files;
  f_files.reserve(jfiles.size());
  std::transform(jfiles.begin(), jfiles.end(), std::back_inserter(f_files),
                 [](auto& file) {
                   auto status =
                       std::filesystem::status(std::filesystem::path{
                           file.as_object().at("path").as_string().c_str()}
                                                   .lexically_normal());
                   if (std::filesystem::is_regular_file(status)) {
                     return storm::File{
                         std::filesystem::path{
                             file.as_object().at("path").as_string().c_str()}
                             .lexically_normal(),
                         // /storage/cms/...
                     };
                   } else {
                     return storm::File{
                         std::filesystem::path{
                             file.as_object().at("path").as_string().c_str()}
                             .lexically_normal(),
                         storm::File::State{storm::File::State::failed},
                         // /storage/cms/...
                     };
                   }
                 });
  std::sort(f_files.begin(), f_files.end(),
            [](storm::File const& a, storm::File const& b) {
              return a.path < b.path;
            });
  return f_files;
}

// Returns a vector of File objects, given a JSON with one or more files listed
// in the "paths" field, for a given cancel request.
std::vector<storm::File> storm::from_json_paths(std::string_view const& body)
{
  std::vector<storm::File> f_files;
  auto const value =
      boost::json::parse(boost::json::string_view{body.data(), body.size()});

  auto& jfiles = value.as_object().at("paths").as_array();
  f_files.reserve(jfiles.size());
  std::transform(
      jfiles.begin(), jfiles.end(), std::back_inserter(f_files),
      [](auto& file) {
        return storm::File{
            std::filesystem::path{file.as_string().c_str()}.lexically_normal(),
        };
      });
  std::sort(f_files.begin(), f_files.end(),
            [](storm::File const& a, storm::File const& b) {
              return a.path < b.path;
            });
  return f_files;
}

std::map<std::string, std::string> storm::get_host(crow::request const& req){
  const std::string header = req.get_header_value("Forwarded");
  std::regex host_match("host=(.*?)(;|$)");
  std::regex proto_match("proto=(.*?)(;|$)");
  std::smatch match;
  std::map<std::string, std::string> header_map;
  if (std::regex_search(header.begin(), header.end(), match, host_match))
      header_map["host"] = match[1];
  if (std::regex_search(header.begin(), header.end(), match, proto_match))
      header_map["proto"] = match[1];

  return header_map;
}