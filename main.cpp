#include "configuration.hpp"
#include "database.hpp"
#include "tape_api.hpp"

int main(int, char*[])
{
  soci::session sql(soci::sqlite3, "storm-tape.db");
  crow::SimpleApp app;
  MockDatabase db;
  Configuration config{};

  create_routes(app, config, db);
  // Configuring server with hostname instead of IP address with Crow?
  app.port(config.port).run();
}
