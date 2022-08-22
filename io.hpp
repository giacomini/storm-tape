#ifndef IO_HPP
#define IO_HPP

#include "file.hpp"
#include <boost/json.hpp>

namespace storm {
class StageRequest;

boost::json::object newStage_to_json(std::string id);
boost::json::object alreadyStaged_to_json(StageRequest const* stage, std::string id);
boost::json::object fileMissing_to_json(std::vector<std::filesystem::path> missing,
                            std::string const& id);
boost::json::array fileNotInArchive_to_json(std::vector<std::filesystem::path> missing,
                           boost::json::array jbody);
boost::json::array infoFromFiles_to_json(std::vector<File> file,
                           boost::json::array jbody);
std::vector<File> files_from_json_path(std::string_view body);
std::vector<File> files_from_json_paths(std::string_view body);

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
  auto const index = to_underlying(locality);
  return index >= 0 && index < std::size(localities) ? localities[index]
                                                     : "UNKNOWN"s;
}
} // namespace storm

#endif