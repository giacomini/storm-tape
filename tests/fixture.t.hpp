#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <filesystem>
#include <fstream>

#include "configuration.hpp"
#include "database_soci.hpp"
#include "local_storage.hpp"
#include "storage.hpp"
#include "tape_service.hpp"

namespace storm {
auto static constexpr DB_NAME           = "storm-tape-test.sqlite";
auto static constexpr DUMMY_CONFIG_PATH = "storm-tape-test.conf";

static inline void make_dummy_config()
{
  std::ofstream out{DUMMY_CONFIG_PATH};
  out << R"(
storage-areas: 
- name: sa1
  root: /tmp
  access-point: /tmp
)";
}

class TestFixture
{
  soci::session m_sql;
  storm::Configuration m_config;
  storm::LocalStorage m_storage;

 protected:
  storm::SociDatabase db;
  storm::TapeService service;

 public:
  TestFixture()
      : m_sql{soci::sqlite3, DB_NAME}
      , m_config{storm::load_configuration([&]() {
        make_dummy_config();
        return DUMMY_CONFIG_PATH;
      }())}
      , m_storage{}
      , db{m_sql}
      , service{m_config, db, m_storage}
  {}

  ~TestFixture()
  {
    std::filesystem::remove(DB_NAME);
    std::filesystem::remove(DUMMY_CONFIG_PATH);
  }
};
} // namespace storm