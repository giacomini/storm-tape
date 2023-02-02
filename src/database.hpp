#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "stage_request.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <map>

namespace storm {

class Database
{
 public:
  virtual std::string insert(StageRequest stage)                = 0;
  virtual StageRequest const* find(std::string const& id) const = 0;
  virtual StageRequest* find(std::string const& id)             = 0;
  virtual int erase(std::string const& id)                      = 0;
};

class MockDatabase : public Database
{
  std::map<std::string, StageRequest> m_db;
  boost::uuids::random_generator m_uuid_gen;

 public:
  std::string insert(StageRequest stage) override
  {
    auto const uuid = m_uuid_gen();
    auto const id   = to_string(uuid);
    auto const ret  = m_db.insert({id, std::move(stage)});
    assert(ret.second == true);
    return id;
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
  int erase(std::string const& id) override
  {
    return m_db.erase(id);
  }
};
} // namespace storm

#endif
