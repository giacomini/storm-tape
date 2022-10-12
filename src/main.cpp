#include "configuration.hpp"
#include "database.hpp"
#include "routes.hpp"
#include "tape_service.hpp"

int main(int, char*[])
{
  soci::session sql(soci::sqlite3, "storm-tape.db");
  crow::SimpleApp app;
  storm::MockDatabase db;
  storm::Configuration config{};
  storm::TapeService service{db};

  create_routes(app, config, service);
  app.port(config.port).run();
}
