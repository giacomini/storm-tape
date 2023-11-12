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
#include <vector>

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

struct StageUpdate
{
  std::optional<StageEntity> stage;
  std::span<std::pair<PhysicalPath, File::State>> files;
  TimePoint tp;
};

class Database
{
  virtual bool
  update(std::span<std::pair<PhysicalPath, File::State>> path_states,
         TimePoint tp)                                 = 0;
  virtual bool update(StageEntity const& stage_entity) = 0;

 public:
  virtual ~Database()                                               = default;
  virtual bool insert(StageId const& id, StageRequest const& stage) = 0;
  virtual std::optional<StageRequest> find(StageId const& id) const = 0;
  virtual std::vector<StageId> find_incomplete_stages() const       = 0;
  virtual bool update(StageId const& id, LogicalPath const& path,
                      File::State state)                            = 0;
  virtual bool update(StageId const& id, LogicalPath const& path,
                      File::State state, TimePoint tp)              = 0;
  virtual bool update(PhysicalPath const& path, File::State state,
                      TimePoint tp)                                 = 0;
  virtual bool update(StageId const& id, std::span<LogicalPath const> paths,
                      File::State state, TimePoint tp)              = 0;
  virtual bool update(std::span<PhysicalPath const> paths, File::State state,
                      TimePoint tp)                                 = 0;
  virtual bool update(StageUpdate const& stage_update)              = 0;
  virtual bool erase(StageId const& id)                             = 0;
  virtual std::size_t count_files(File::State state) const          = 0;
  virtual PhysicalPaths get_files(File::State state,
                                  std::size_t n_files) const        = 0;
};

} // namespace storm

#endif
