#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "stage_request.hpp"
#include "storage.hpp"
#include <algorithm>
#include <cassert>
#include <map>
#include <numeric>
#include <optional>
#include <span>

namespace storm {

using StageId   = std::string;
using Filename  = std::string;
using TimePoint = long long;

struct StageEntity
{
  StageId id;
  TimePoint created_at{0};
  TimePoint started_at{0};
  TimePoint completed_at{0};
};

struct FileEntity
{
  StageId stage_id;
  Filename logical_path;
  Filename physical_path;
  File::State state{File::State::submitted};
  Locality locality{Locality::unavailable}; // TODO is it needed in the db?
  TimePoint started_at{0};
  TimePoint finished_at{0};
};

class Database
{
 public:
  virtual ~Database()                                               = default;
  virtual bool insert(StageId const& id, StageRequest const& stage) = 0;
  virtual std::optional<StageRequest> find(StageId const& id) const = 0;
  virtual bool update(StageId const& id, Path const& logical_path,
                      File::State state)                            = 0;
  virtual bool update(Path const& physical_path, File::State state,
                      TimePoint tp)                                 = 0;
  virtual bool update(std::span<Path const> physical_paths, File::State state,
                      TimePoint tp)                                 = 0;
  virtual bool erase(StageId const& id)                             = 0;
  virtual std::size_t count_files(File::State state) const          = 0;
  // get physical paths
  virtual Paths get_files(File::State state, std::size_t n_files) const = 0;
};

class MockDatabase : public Database
{
  std::map<StageId, StageRequest> m_db;

 public:
  bool insert(StageId const& id, const StageRequest& stage) override
  {
    auto const [it, inserted] = m_db.try_emplace(id, stage);
    return inserted;
  }

  std::optional<StageRequest> find(StageId const& id) const override
  {
    auto it = m_db.find(id);
    return it == m_db.end() ? std::nullopt : std::optional{it->second};
  }

  bool update(StageId const& id, Path const& logical_path, File::State state) override
  {
    auto it = m_db.find(id);
    if (it == m_db.end()) {
      return false;
    }
    auto& stage = it->second;
    auto file_it =
        std::lower_bound(stage.files.begin(), stage.files.end(), File{logical_path},
                         [](File const& file1, File const& file2) {
                           return file1.logical_path.string() < file2.logical_path.string();
                         });
    if (file_it->logical_path == logical_path) {
      file_it->state = state;
      return true;
    }

    return false;
  }

  bool update(Path const& physical_path, File::State state, TimePoint tp) override
  {
    for (auto&& [id, stage] : m_db) {
      auto it =
          std::lower_bound(stage.files.begin(), stage.files.end(), File{physical_path},
                           [](File const& file1, File const& file2) {
                             return file1.physical_path.string() < file2.physical_path.string();
                           });
      if (it->physical_path == physical_path) {
        it->state = state;
        switch (state) {
        case File::State::started:
          it->started_at = tp;
          break;
        case File::State::completed:
        case File::State::cancelled:
        case File::State::failed:
          it->finished_at = tp;
          break;
        case File::State::submitted:
          // this transition is not foreseen, ignore
          break;
        default:
          assert(false);
        }
      }
    }

    return true;
  }

  bool update(std::span<Path const>, File::State, TimePoint) override
  {
    return true;
  }

  bool erase(StageId const& id) override
  {
    return m_db.erase(id) == 1;
  }

  std::size_t count_files(File::State state) const override
  {
    auto count = std::accumulate( //
        m_db.begin(), m_db.end(), 0, [&](auto acc, auto const& v) {
          StageRequest const& req = v.second;
          return acc
               + std::count_if(
                     req.files.begin(), req.files.end(),
                     [&](File const& file) { return file.state == state; });
        });
    return static_cast<std::size_t>(count);
  }

  Paths get_files(File::State state, std::size_t n_files) const override
  {
    Paths result;
    result.reserve(std::min(n_files, std::size_t{1'000}));
    for (auto&& [id, stage] : std::as_const(m_db)) {
      for (auto&& file : stage.files) {
        if (file.state == state) {
          result.push_back(file.physical_path);
        }
      }
    }
    return result;
  }
};

} // namespace storm

#endif
