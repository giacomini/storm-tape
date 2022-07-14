#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <boost/json.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <filesystem>
#include <map>

namespace fs   = std::filesystem;
namespace json = boost::json;

using Clock = std::chrono::system_clock;

struct File
{
  enum class State : unsigned char
  {
    submitted,
    started,
    cancelled,
    failed,
    completed
  };
  enum class Locality : unsigned char
  {
    unknown,
    on_tape = 1,
    on_disk = 2
  };
  fs::path path;
  Locality locality{Locality::on_tape};
  State state{State::submitted};
};

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

// FG should a path be absolute? can there be duplicates?
struct Stage
{
  Clock::time_point created_at;
  Clock::time_point started_at;
  std::vector<File> files;
  Stage(std::string_view body)
      : created_at(Clock::now())
      , started_at(created_at)
  {
    auto const value = json::parse(json::string_view{body.data(), body.size()});
    auto& jfiles     = value.as_object().at("files").as_array();
    files.reserve(jfiles.size());
    std::transform(
        jfiles.begin(), jfiles.end(), std::back_inserter(files),
        [](auto& file) {
          return File{
              fs::path{file.as_object().at("path").as_string().c_str()},
              // /storage/cms/...
          };
        });
    std::sort(files.begin(), files.end(),
              [](File const& a, File const& b) { return a.path < b.path; });
  }
};

struct RequestWithPaths
{
  std::vector<fs::path> paths;
  RequestWithPaths(std::string_view body)
  {
    auto const value = json::parse(json::string_view{body.data(), body.size()});
    auto& jpaths     = value.as_object().at("paths").as_array();
    paths.reserve(jpaths.size());
    std::transform(
        jpaths.begin(), jpaths.end(), std::back_inserter(paths),
        [](auto& path) { return fs::path{path.as_string().c_str()}; });
    std::sort(paths.begin(), paths.end());
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

class Database
{
  public:
  virtual std::string insert(Stage stage) = 0;
  virtual Stage const* find(std::string const& id) const = 0;
  virtual Stage* find(std::string const& id) = 0;
  virtual int erase(std::string const& id) = 0;
};

class MockDatabase : public Database
{
  std::map<std::string, Stage> m_db;
  boost::uuids::random_generator m_uuid_gen;

 public:
  std::string insert(Stage stage) override
  {
    auto const uuid = m_uuid_gen();
    auto const id   = to_string(uuid);
    auto const ret  = m_db.insert({id, std::move(stage)});
    assert(ret.second == true);
    return id;
  }
  Stage const* find(std::string const& id) const override
  {
    auto it = m_db.find(id);
    return it == m_db.end() ? nullptr : &it->second;
  }
  Stage* find(std::string const& id) override
  {
    auto it = m_db.find(id);
    return it == m_db.end() ? nullptr : &it->second;
  }
  int erase(std::string const& id) override
  {
    return m_db.erase(id);
  }
};

#endif
