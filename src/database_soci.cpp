#include "database_soci.hpp"
#include "io.hpp"
#include "sql_queries.hpp"

namespace soci {
template<>
struct type_conversion<storm::StageEntity>
{
  using base_type = values;
  static void from_base(const soci::values& v, soci::indicator ind,
                        storm::StageEntity& req)
  {
    using namespace std::chrono;
    req.id = v.get<storm::StageId>("id");

    req.created_at =
        storm::TimePoint{std::chrono::seconds{v.get<long long>("created_at")}};
    req.started_at =
        storm::TimePoint{std::chrono::seconds{v.get<long long>("started_at")}};
  }

  static void to_base(const storm::StageEntity& req, soci::values& v,
                      soci::indicator ind)
  {
    using namespace std::chrono;
    long long created_at =
        duration_cast<seconds>(req.created_at.time_since_epoch()).count();
    long long started_at =
        duration_cast<seconds>(req.started_at.time_since_epoch()).count();
    v.set("id", req.id);
    v.set("created_at", created_at);
    v.set("started_at", started_at);
    ind = i_ok;
  }
};

template<>
struct type_conversion<storm::FileEntity>
{
  using base_type = values;
  static void from_base(const soci::values& v, soci::indicator ind,
                        storm::FileEntity& file)
  {
    file.stage_id = v.get<storm::StageId>("stage_id");
    file.path     = v.get<storm::Filename>("path");
    file.state    = static_cast<storm::File::State>(v.get<int>("state"));
    file.locality = static_cast<storm::File::Locality>(v.get<int>("locality"));
  }

  static void to_base(const storm::FileEntity& file, soci::values& v,
                      soci::indicator ind)
  {
    v.set("stage_id", file.stage_id);
    v.set("path", file.path);
    v.set("locality", storm::to_underlying(file.locality));
    v.set("state", storm::to_underlying(file.state));
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

  if (n_files == 0)
    return files;
  files.reserve(n_files);

  soci::rowset<FileEntity> rs_f =
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

bool SociDatabase::insert(StageId const& id, StageRequest stage)
{
  using namespace std::chrono;
  StageEntity s_entity{id, stage.created_at(), stage.started_at()};

  try {
    // Insert stage
    m_sql << storm::sql::Stage::INSERT, soci::use(s_entity);

    // Insert files
    const auto& files = stage.files();
    std::for_each(files.begin(), files.end(), [&](auto const f) {
      FileEntity fileEntity{id, f.path, f.state, f.locality};
      m_sql << storm::sql::File::INSERT, soci::use(fileEntity);
    });
  } catch (soci::sqlite3_soci_error e) {
    std::cerr << "Sqlite3 error: " << e.what() << '\n';
    return false;
  } catch (std::exception e) {
    std::cerr << "Uncaught exception: " << e.what() << '\n';
    return false;
  }
  return true;
}

std::optional<StageRequest const> SociDatabase::find(StageId const& id) const
{
  StageEntity s_entity{};
  m_sql << storm::sql::Stage::FIND, soci::into(s_entity), soci::use(id);

  if (s_entity.id != id)
    return std::nullopt;

  const auto f_entities = find_file_entities(id, m_sql);
  std::vector<File> files;
  std::transform(f_entities.begin(), f_entities.end(),
                 std::back_inserter(files), [](auto& fe) {
                   return File{fe.path, fe.state, fe.locality};
                 });

  return std::optional<StageRequest>{StageRequest{
      files}}; // FIXME: created_at and started_at cannot be initialized
}

std::optional<StageRequest> SociDatabase::find(StageId const& id)
{
  StageEntity s_entity{};
  m_sql << storm::sql::Stage::FIND, soci::into(s_entity), soci::use(id);

  if (s_entity.id != id)
    return std::nullopt;

  const auto f_entities = find_file_entities(id, m_sql);
  std::vector<File> files;
  files.reserve(f_entities.size());
  std::transform(f_entities.begin(), f_entities.end(),
                 std::back_inserter(files), [](auto& fe) {
                   return File{fe.path, fe.state, fe.locality};
                 });

  return std::optional<StageRequest>{StageRequest{
      files}}; // FIXME: created_at and started_at cannot be initialized
}

bool SociDatabase::erase(StageId const& id)
{
  try {
    auto stage = find(id);
    if (!stage.has_value())
      return false;

    // Delete all file records
    auto f_entities = find_file_entities(id, m_sql);
    for (const auto& f : f_entities) {
      m_sql << storm::sql::File::DELETE, soci::use(f);
    }
    // Delete stage record
    m_sql << storm::sql::Stage::DELETE, soci::use(id);
  } catch (soci::sqlite3_soci_error e) {
    std::cerr << "Sqlite3 error: " << e.what() << '\n';
    return false;
  } catch (std::exception e) {
    std::cerr << "Uncaught exception: " << e.what() << '\n';
    return false;
  }
  return true;
}
} // namespace storm