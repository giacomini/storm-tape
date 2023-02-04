#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "stage_request.hpp"
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <map>

namespace storm {

class Database
{
 public:
  virtual bool insert(std::string const& id, StageRequest stage)                = 0;
  virtual StageRequest const* find(std::string const& id) const = 0;
  virtual StageRequest* find(std::string const& id)             = 0;
  virtual bool erase(std::string const& id)                      = 0;
};

class MockDatabase : public Database
{
  std::map<std::string, StageRequest> m_db;

 public:
  bool insert(std::string const& id, StageRequest stage) override
  {
    auto const ret  = m_db.insert({id, std::move(stage)});
    return ret.second == true;
  }
  StageRequest const* find(std::string const& id) const override
  {
    auto it = m_db.find(id);
    return it == m_db.end() ? nullptr : &it->second;
  }
  StageRequest* find(std::string const& id) override
  {
    auto it = m_db.find(id);
    return it == m_db.end() ? nullptr : &it->second;
  }
  bool erase(std::string const& id) override
  {
    return m_db.erase(id) == 1;
  }
};
} // namespace storm

#endif
