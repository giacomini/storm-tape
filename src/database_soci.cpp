#include "database_soci.hpp"
#include "io.hpp"
#include "profiler.hpp"
#include "sql_queries.hpp"
#include <iostream>

namespace soci {
template<>
struct type_conversion<storm::StageEntity>
{
  using base_type = soci::values;
  static void from_base(const soci::values& v, soci::indicator,
                        storm::StageEntity& req)
  {
    req.id = v.get<storm::StageId>("id");

    req.created_at   = v.get<storm::TimePoint>("created_at");
    req.started_at   = v.get<storm::TimePoint>("started_at");
    req.completed_at = v.get<storm::TimePoint>("completed_at");
  }

  static void to_base(const storm::StageEntity& req, soci::values& v,
                      soci::indicator& ind)
  {
    v.set("id", req.id);
    v.set("created_at", req.created_at);
    v.set("started_at", req.started_at);
    v.set("completed_at", req.completed_at);
    ind = i_ok;
  }
};

template<>
struct type_conversion<storm::FileEntity>
{
  using base_type = values;
  static void from_base(const soci::values& v, soci::indicator,
                        storm::FileEntity& file)
  {
    file.stage_id      = v.get<storm::StageId>("stage_id");
    file.logical_path  = v.get<storm::Filename>("logical_path");
    file.physical_path = v.get<storm::Filename>("physical_path");
    file.state         = static_cast<storm::File::State>(v.get<int>("state"));
    file.locality      = storm::Locality::unavailable;
    file.started_at    = v.get<storm::TimePoint>("started_at");
    file.finished_at   = v.get<storm::TimePoint>("finished_at");
  }

  static void to_base(const storm::FileEntity& file, soci::values& v,
                      soci::indicator& ind)
  {
    v.set("stage_id", file.stage_id);
    v.set("logical_path", file.logical_path);
    v.set("physical_path", file.physical_path);
    v.set("state", storm::to_underlying(file.state));
    using uchar = unsigned char;
    v.set("locality", uchar{});
    v.set("started_at", file.started_at);
    v.set("finished_at", file.finished_at);
    ind = i_ok;
  }
};
} // namespace soci

namespace storm {
std::vector<FileEntity> find_file_entities(const StageId& id,
                                           soci::session& sql)
{
  size_t n_files{0};
  std::vector<FileEntity> files{};
  sql << storm::sql::File::COUNT_BY_STAGE_ID, soci::use(id),
      soci::into(n_files);

  if (n_files == 0) {
    return files;
  }
  files.reserve(n_files);

  soci::rowset<FileEntity> const rs_f =
      (sql.prepare << storm::sql::File::FIND_BY_STAGE_ID, soci::use(id));
  std::for_each(rs_f.begin(), rs_f.end(),
                [&](auto& f) { files.push_back(std::move(f)); });
  return files;
}

// ---------------------
// SociDatabase

SociDatabase::SociDatabase(soci::session& sql)
    : m_sql{sql}
{
  m_sql << storm::sql::Stage::CREATE_IF_NOT_EXISTS;
  m_sql << storm::sql::File::CREATE_IF_NOT_EXISTS;
}

bool SociDatabase::insert(StageId const& id, StageRequest const& stage)
{
  PROFILE_FUNCTION();
  StageEntity s_entity{id, stage.created_at, stage.started_at,
                       stage.completed_at};

  try {
    soci::transaction tr{m_sql};

    // Insert stage
    m_sql << storm::sql::Stage::INSERT, soci::use(s_entity);

    // Insert files
    const auto& files = stage.files;
    std::for_each(files.begin(), files.end(), [&](auto const& f) {
      FileEntity const entity{id,           f.logical_path, f.physical_path,
                              f.state,      f.locality,     f.started_at,
                              f.finished_at};
      m_sql << storm::sql::File::INSERT, soci::use(entity);
    });

    tr.commit();
  } catch (soci::soci_error const& e) {
    std::cerr << "Soci error: " << e.what() << '\n';
    return false;
  }
  return true;
}

std::optional<StageRequest> SociDatabase::find(StageId const& id) const
{
  PROFILE_FUNCTION();
  StageEntity s_entity{};
  m_sql << storm::sql::Stage::FIND, soci::into(s_entity), soci::use(id);

  if (s_entity.id != id) {
    return std::nullopt;
  }

  const auto f_entities = find_file_entities(id, m_sql);
  Files files;
  files.reserve(f_entities.size());
  std::transform(f_entities.begin(), f_entities.end(),
                 std::back_inserter(files), [](auto& fe) {
                   return File{fe.logical_path, fe.physical_path,
                               fe.state,        fe.locality,
                               fe.started_at,   fe.finished_at};
                 });

  return StageRequest{std::move(files), s_entity.created_at,
                      s_entity.started_at, s_entity.completed_at};
}

std::vector<StageId> SociDatabase::find_incomplete_stages() const
{
  PROFILE_FUNCTION();

  std::size_t n_stages{0};
  m_sql << "SELECT COUNT(*) FROM Stage WHERE completed_at = 0",
      soci::into(n_stages);

  if (n_stages == 0) {
    return {};
  }

  std::vector<StageId> result(n_stages);
  m_sql << "SELECT id FROM Stage WHERE completed_at = 0", soci::into(result);

  // NB an incomplete stage is a stage whose files are not all in a final state;
  // in such cases the completed_at timestamp is 0. Since the DB contains stale
  // information, which is reconciled with reality only when a status is called,
  // the result may include stages that are actually completed; this is not a
  // problem for the current use, because the important thing is that the result
  // includes all incomplete stages

  return result;
}

bool SociDatabase::update(StageId const& id, LogicalPath const& path,
                          File::State state)
{
  PROFILE_FUNCTION();
  try {
    auto const cstate = to_underlying(state);
    auto const cpath  = path.string();
    m_sql << "UPDATE File SET state = :state WHERE stage_id = :id AND "
             "logical_path = :logical_path;",
        soci::use(cstate), soci::use(id), soci::use(cpath);
  } catch (soci::soci_error const& e) {
    std::cerr << "Soci error: " << e.what() << '\n';
    return false;
  }
  return true;
}

bool SociDatabase::update(StageId const& id, LogicalPath const& path,
                          File::State state, TimePoint tp)
{
  PROFILE_FUNCTION();
  try {
    auto const cstate = to_underlying(state);
    auto const cpath  = path.string();
    switch (state) {
    case File::State::started: {
      m_sql << "UPDATE File SET state = :state, started_at = :tp "
               "WHERE stage_id = :id AND logical_path = :logical_path;",
          soci::use(cstate), soci::use(tp), soci::use(id), soci::use(cpath);
      break;
    }
    case File::State::completed:
    case File::State::cancelled:
    case File::State::failed: {
      m_sql << "UPDATE File SET state = :state, "
               "started_at = CASE WHEN started_at = 0 THEN :tp_start ELSE "
               "started_at END, "
               "finished_at = :tp_end "
               "WHERE stage_id = :id AND logical_path = :logical_path;",
          soci::use(cstate), soci::use(tp), soci::use(tp), soci::use(id),
          soci::use(cpath);
      break;
    }
    case File::State::submitted:
      // this transition is not foreseen, ignore
      break;
    default:
      assert(false && "invalid state");
    }

  } catch (soci::soci_error const& e) {
    std::cerr << "Soci error: " << e.what() << '\n';
    return false;
  }
  return true;
}

bool SociDatabase::update(PhysicalPath const& path, File::State state,
                          TimePoint tp)
{
  PROFILE_FUNCTION();
  try {
    auto const new_state       = to_underlying(state);
    auto const submitted_state = to_underlying(File::State::submitted);
    auto const started_state   = to_underlying(File::State::started);
    auto const cpath           = path.string();

    switch (state) {
    case File::State::started: {
      using soci::use;
      m_sql << "UPDATE File SET state = :state, started_at = :tp WHERE "
               "physical_path = :physical_path AND state = :submitted;",
          use(new_state), use(tp), use(cpath), use(submitted_state);
      break;
    }
    case File::State::completed:
    case File::State::cancelled:
    case File::State::failed: {
      using soci::use;
      m_sql << "UPDATE File SET state = :state, "
               "started_at = CASE WHEN started_at = 0 THEN :tp_start ELSE "
               "started_at END, "
               "finished_at = :tp_end "
               "WHERE physical_path = :physical_path AND state IN (:submitted, "
               ":started);",
          use(new_state), use(tp), use(tp), use(cpath), use(submitted_state),
          use(started_state);
      break;
    }
    case File::State::submitted:
      // this transition is not foreseen, ignore
      break;
    default:
      assert(false && "invalid state");
    }
  } catch (soci::soci_error const& e) {
    std::cerr << "SOCI error: " << e.what() << '\n';
    return false;
  }
  return true;
}

bool SociDatabase::update(StageId const& id, std::span<LogicalPath const> paths,
                          File::State state, TimePoint tp)
{
  PROFILE_FUNCTION();
  soci::transaction tr{m_sql};
  std::for_each(paths.begin(), paths.end(),
                [&](auto& p) { update(id, p, state, tp); });
  tr.commit();
  return true;
}

bool SociDatabase::update(std::span<PhysicalPath const> paths,
                          File::State state, TimePoint tp)
{
  PROFILE_FUNCTION();
  soci::transaction tr{m_sql};
  std::for_each(paths.begin(), paths.end(),
                [&](auto& p) { update(p, state, tp); });
  tr.commit();
  return true;
}

bool SociDatabase::update(
    std::span<std::pair<PhysicalPath, File::State>> path_states, TimePoint tp)
{
  PROFILE_FUNCTION();
  for (auto const& [path, state] : path_states) {
    update(path, state, tp);
  }
  return true;
}

bool SociDatabase::update(StageEntity const& entity)
{
  PROFILE_FUNCTION();
  using namespace soci;
  m_sql << "UPDATE Stage SET created_at = :created_at, "
           "started_at = :started_at, completed_at = :completed_at "
           "WHERE id = :id;",
      use(entity.created_at), use(entity.started_at), use(entity.completed_at),
      use(entity.id);
  return true;
}

bool SociDatabase::update(StageUpdate const& stage_update)
{
  PROFILE_FUNCTION();
  soci::transaction tr{m_sql};
  if (stage_update.stage.has_value()) {
    update(*stage_update.stage);
  }
  update(stage_update.files, stage_update.tp);
  tr.commit();
  return true;
}

std::size_t SociDatabase::count_files(File::State state) const
{
  PROFILE_FUNCTION();
  std::size_t count{};
  auto const cstate = to_underlying(state);
  m_sql
      << "SELECT COUNT(DISTINCT physical_path) FROM File WHERE state = :state;",
      soci::into(count), soci::use(cstate);
  return std::size_t{count};
}

PhysicalPaths SociDatabase::get_files(File::State state,
                                      std::size_t n_files) const
{
  PROFILE_FUNCTION();
  std::vector<Filename> filenames(n_files);
  auto const cstate = to_underlying(state);

  m_sql << "SELECT DISTINCT physical_path FROM File WHERE state = :state LIMIT "
           ":n_files;",
      soci::into(filenames), soci::use(cstate), soci::use(n_files);

  PhysicalPaths result;
  result.reserve(filenames.size());
  std::transform(
      filenames.begin(), filenames.end(), std::back_inserter(result),
      [](auto& filename) { return PhysicalPath{std::move(filename)}; });
  return result;
}

bool SociDatabase::erase(StageId const& id)
{
  PROFILE_FUNCTION();
  try {
    int count{0};
    m_sql << "SELECT count(*) FROM Stage WHERE id = :id;", soci::into(count),
        soci::use(id);
    if (count == 0) {
      return false;
    }

    m_sql << "DELETE FROM File WHERE stage_id = :stage_id;", soci::use(id);
    m_sql << "DELETE FROM Stage WHERE id = :id", soci::use(id);

  } catch (soci::soci_error const& e) {
    std::cerr << "Soci error: " << e.what() << '\n';
    return false;
  }

  return true;
}
} // namespace storm
