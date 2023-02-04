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
  virtual bool insert(std::string const& id, StageRequest stage)                = 0;
  virtual StageRequest const* find(std::string const& id) const = 0;
  virtual StageRequest* find(std::string const& id)             = 0;
  virtual int erase(std::string const& id)                      = 0;
};

class MockDatabase : public Database
{
  std::map<std::string, StageRequest> m_db;
  boost::uuids::random_generator m_uuid_gen;

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
  int erase(std::string const& id) override
  {
    return m_db.erase(id);
  }
};
} // namespace storm

#endif
