#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "stage_request.hpp"
#include <map>
#include <optional>

namespace storm {

using StageId   = std::string;
using Filename  = std::string;
using TimePoint = std::chrono::system_clock::time_point;

struct StageEntity
{
  StageId id;
  TimePoint created_at;
  TimePoint started_at;
};

struct FileEntity
{
  StageId stage_id;
  Filename path;
  File::State state;
  File::Locality locality;
};

class Database
{
 public:
  virtual ~Database() = default;
  virtual bool insert(std::string const& id, StageRequest const& stage) = 0;
  virtual std::optional<StageRequest const>
  find(std::string const& id) const                               = 0;
  virtual std::optional<StageRequest> find(std::string const& id) = 0;
  virtual bool erase(std::string const& id)                       = 0;
};

class MockDatabase : public Database
{
  std::map<std::string, StageRequest> m_db;

 public:
  bool insert(std::string const& id, const StageRequest& stage) override
  {
    auto const ret = m_db.insert({id, stage});
    return ret.second;
  }
  std::optional<StageRequest const> find(std::string const& id) const override
  {
    auto it = m_db.find(id);
    return it == m_db.end() ? std::nullopt : std::optional(it->second);
  }
  std::optional<StageRequest> find(std::string const& id) override
  {
    auto it = m_db.find(id);
    return it == m_db.end() ? std::nullopt : std::optional(it->second);
  }
  bool erase(std::string const& id) override
  {
    return m_db.erase(id) == 1;
  }
};
} // namespace storm

#endif
