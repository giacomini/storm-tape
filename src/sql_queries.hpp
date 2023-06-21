#ifndef STORM_TAPE_SQL_QUERIES_H
#define STORM_TAPE_SQL_QUERIES_H

namespace storm::sql {
// ---------------------
// Stage Table
namespace Stage {
static constexpr auto CREATE_IF_NOT_EXISTS = R"(
  CREATE TABLE IF NOT EXISTS Stage (
    id           TEXT   PRIMARY KEY,
    created_at   BIGINT NOT NULL,
    started_at   BIGINT NOT NULL,
    completed_at BIGINT NOT NULL
  );
)";

static constexpr auto DROP_IF_EXISTS = R"(
  DROP TABLE IF EXISTS Stage;
)";

static constexpr auto INSERT = R"(
  INSERT INTO Stage VALUES (:id, :created_at, :started_at, :completed_at)
)";

static constexpr auto FIND = R"(
  SELECT * FROM Stage WHERE id = :id
)";

static constexpr auto SQL_GET_ALL_STAGE_IDS = R"(
  SELECT id FROM Stage;
)";

static constexpr auto DELETE = R"(
  DELETE FROM Stage WHERE id = :id
)";
} // namespace Stage

namespace File {
static constexpr auto CREATE_IF_NOT_EXISTS = R"(
  CREATE TABLE IF NOT EXISTS File (
    stage_id      TEXT    NOT NULL,
    logical_path  TEXT    NOT NULL,
    physical_path TEXT    NOT NULL,
    state         INTEGER NOT NULL,
    locality      INTEGER NOT NULL,
    started_at    BIGINT  NOT NULL,
    finished_at   BIGINT  NOT NULL,
    PRIMARY KEY (stage_id, logical_path),
    FOREIGN KEY(stage_id) REFERENCES Stage(id)
  );
)";

static constexpr auto DROP_IF_EXISTS = R"(
  DROP TABLE IF EXISTS FILE;
)";

static constexpr auto INSERT = R"(
  INSERT INTO File VALUES (:stage_id, :logical_path, :physical_path, :state, :locality, :started_at, :finished_at)
)";

static constexpr auto COUNT_BY_STAGE_ID = R"(
  SELECT COUNT(*) FROM File WHERE stage_id = :stage_id
)";

static constexpr auto FIND_BY_STAGE_ID = R"(
  SELECT * FROM File WHERE stage_id = :stage_id ORDER BY logical_path
)";

static constexpr auto FIND = R"(
  SELECT * FROM File WHERE stage_id = :stage_id AND logical_path = :logical_path)
)";

static constexpr auto DELETE = R"(
  DELETE FROM File WHERE stage_id = :stage_id AND logical_path = :logical_path
)";
} // namespace File
} // namespace storm::sql
#endif // STORM_TAPE_SQL_QUERIES_H
