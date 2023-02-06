//
// Created by Jacopo Gasparetto on 06/02/23.
//

#ifndef STORM_TAPE_DATABASE_SOCI_HPP
#define STORM_TAPE_DATABASE_SOCI_HPP

#include "database.hpp"

#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>

namespace storm {

class SociDatabase : public Database
{
  soci::session& m_sql;

 public:
  explicit SociDatabase(soci::session& sql);
  bool insert(StageId const& id, StageRequest stage) override;
  std::optional<StageRequest> find(std::string const& id) override;
  std::optional<StageRequest const> find(std::string const& id) const override;
  virtual bool erase(std::string const& id) override;
};

} // namespace storm

#endif // STORM_TAPE_DATABASE_SOCI_HPP
