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
  bool insert(StageId const& id, StageRequest const& stage) override;
  std::optional<StageRequest> find(std::string const& id) const override;
  bool update(StageId const& id, Path const& path, File::State state) override;
  std::size_t update(Path const& path, File::State state, TimePoint tp) override;
  bool erase(std::string const& id) override;
  std::size_t count_files(File::State state) const override;
  std::vector<Filename> get_files(File::State state, std::size_t n_files) const override;
};

} // namespace storm

#endif // STORM_TAPE_DATABASE_SOCI_HPP
