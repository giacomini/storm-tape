#include "configuration.hpp"
#include "database.hpp"
#include "tape_api.hpp"

int main(int, char*[])
{
  soci::session sql(soci::sqlite3, "storm-tape.db");
  crow::SimpleApp app;
  storm::TapeService service{db};

  create_routes(app, config, service);
  app.port(config.port).run();
}
