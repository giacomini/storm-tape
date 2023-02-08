#ifndef IO_HPP
#define IO_HPP

#include "file.hpp"
#include <boost/json.hpp>
#include <crow.h>
#include <regex>

namespace storm {

class StageRequest;
class StageResponse;
class StatusResponse;
class CancelResponse;
class DeleteResponse;
class ReleaseResponse;
class ArchiveResponse;
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
boost::json::array archive_to_json(std::vector<File> const& file,
                                   boost::json::array& jbody);
crow::response to_crow_response(ArchiveResponse const& resp);

std::vector<File> from_json(std::string_view const& body);
std::vector<File> from_json_paths(std::string_view const& body);

HostInfo get_host(crow::request const& req, Configuration const& conf);

template<class Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
  return static_cast<std::underlying_type_t<Enum>>(e);
}

inline std::string to_string(File::State state)
{
  switch (state) {
  case File::State::submitted:
    return "SUBMITTED";
  case File::State::started:
    return "STARTED";
  case File::State::cancelled:
    return "CANCELLED";
  case File::State::failed:
    return "FAILED";
  case File::State::completed:
    return "COMPLETED";
  default:
    return "UNKNOWN";
  }
}

inline std::string to_string(File::Locality locality)
{
  using namespace std::string_literals;
  static std::string const localities[]{"UNKNOWN"s, "TAPE"s, "DISK"s,
                                        "DISK_AND_TAPE"s};
  std::size_t const index = to_underlying(locality);
  return index < std::size(localities) ? localities[index] : "UNKNOWN"s;
}
} // namespace storm

#endif