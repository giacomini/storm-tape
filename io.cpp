#include "io.hpp"
#include "stage_request.hpp"

// Creates a JSON object with the given request ID, for a new stage request.
boost::json::object storm::newStage_to_json(std::string id)
{
  boost::json::object jbody{{"requestId", id}};
  return jbody;
}

// Creates a JSON object for an already staged request, with a certain
// requestId.
boost::json::object storm::alreadyStaged_to_json(StageRequest const* stage,
                                                 std::string id)
{
  boost::json::array files;
  auto m_files = stage->getFiles();
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
          stage->created_at.time_since_epoch())
          .count();
  jbody["started_at"] = //
      std::chrono::duration_cast<std::chrono::seconds>(
          stage->started_at.time_since_epoch())
          .count();
  jbody["files"] = files;

  return jbody;
}

// Creates a JSON object when one or more files targeted for cancellation do
// not belong to the initially submitted stage request.
boost::json::object
storm::fileMissing_to_json(std::vector<std::filesystem::path> missing,
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
      {"detail", "Files " + sfile + "do not belong to stage request " + id}};
  return jbody;
}

// Given a JSON array, appends the missing or not accessible files in JSON objects, one per file.
boost::json::array
storm::fileNotInArchive_to_json(std::vector<std::filesystem::path> missing,
                                boost::json::array jbody)
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

// Given a JSON array, appends the files information in JSON objects, one per file.
boost::json::array
storm::infoFromFiles_to_json(std::vector<storm::File> files,
                             boost::json::array jbody)
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

// Returns a vector of File objects, given a JSON with one or more files listed
// in each "path" field of the "files" array, for a given stage request.
std::vector<storm::File> storm::files_from_json_path(std::string_view body)
{
  auto const value =
      boost::json::parse(boost::json::string_view{body.data(), body.size()});

  auto& jfiles = value.as_object().at("files").as_array();
  std::vector<storm::File> f_files;
  f_files.reserve(jfiles.size());
  std::transform(jfiles.begin(), jfiles.end(), std::back_inserter(f_files),
                 [](auto& file) {
                   return storm::File{
                       std::filesystem::path{
                           file.as_object().at("path").as_string().c_str()},
                       // /storage/cms/...
                   };
                 });
  std::sort(f_files.begin(), f_files.end(),
            [](File const& a, File const& b) { return a.path < b.path; });
  return f_files;
}

// Returns a vector of File objects, given a JSON with one or more files listed
// in the "paths" field, for a given cancel request.
std::vector<storm::File> storm::files_from_json_paths(std::string_view body)
{
  std::vector<storm::File> f_files;
  auto const value =
      boost::json::parse(boost::json::string_view{body.data(), body.size()});

  auto& jfiles = value.as_object().at("paths").as_array();
  f_files.reserve(jfiles.size());
  std::transform(jfiles.begin(), jfiles.end(), std::back_inserter(f_files),
                 [](auto& file) {
                   return storm::File{
                       std::filesystem::path{file.as_string().c_str()},
                   };
                 });
  std::sort(f_files.begin(), f_files.end(),
            [](File const& a, File const& b) { return a.path < b.path; });
  return f_files;
}