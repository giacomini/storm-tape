#include "database.hpp"
#include "tape_api.hpp"
#include "configuration.hpp"

int main(int, char*[])
{
  soci::session sql(soci::sqlite3, "storm-tape.db");
  crow::SimpleApp app;
  MockDatabase db;
  Configuration const& config{};
  create_routes(app, config, db);
}
