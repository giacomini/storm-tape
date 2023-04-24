#ifndef STORM_TAPE_DATABASE_SOCI_HPP
#define STORM_TAPE_DATABASE_SOCI_HPP

#include "database.hpp"

#include <soci/soci.h>

namespace storm {

class SociDatabase : public Database
{
  soci::session& m_sql;

 public:
  explicit SociDatabase(soci::session& sql);
  bool insert(StageId const& id, StageRequest const& stage) override;
  std::optional<StageRequest> find(std::string const& id) const override;
  bool update(StageId const& id, Path const& logical_path, File::State state) override;
  bool update(Path const& physical_path, File::State state, TimePoint tp) override;
  bool update(std::span<Path const> physical_paths, File::State state, TimePoint tp) override;
  bool update(std::span<std::pair<Path, File::State>> physical_path_states, TimePoint tp) override;
  bool erase(std::string const& id) override;
  std::size_t count_files(File::State state) const override;
  Paths get_files(File::State state, std::size_t n_files) const override;
};

} // namespace storm

#endif // STORM_TAPE_DATABASE_SOCI_HPP
