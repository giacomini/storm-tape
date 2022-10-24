#include "configuration.hpp"
#include "database.hpp"
#include "routes.hpp"
#include "tape_service.hpp"

#ifdef ENABLE_TESTING

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "../test/doctest.h"
#include "io.hpp"
#include "stage_response.hpp"

TEST_CASE("Check IO")
{
  CHECK(storm::from_json("{\"files\":[{\"path\":\"/foo/bar\"}]}").size() == 1);
  CHECK(storm::from_json("{\"files\":[{\"path\":\"/foo/bar\"},{\"path\":\"/bar/foo\"}]}").size() == 2);
  CHECK(storm::from_json("{\"files\":[{\"path\":\"/foo//bar\"}]}").at(0).path.lexically_normal() == "/foo/bar");
  CHECK(storm::from_json("{\"files\":[{\"path\":\"/foo/.///bar/../\"}]}").at(0).path.lexically_normal() == "/foo/");
  CHECK_THROWS_AS(storm::from_json("{ }"), std::exception);
  CHECK_THROWS_AS(storm::from_json(""), std::exception);
  CHECK(storm::from_json_paths("{\"paths\":[\"/foo/bar\"]}").size() == 1);
  CHECK(storm::from_json_paths("{\"paths\":[\"/foo/bar\",\"/bar/foo\"]}").size() == 2);
  CHECK(storm::from_json_paths("{\"paths\":[\"/foo//bar\"]}").at(0).path.lexically_normal() == "/foo/bar");
  CHECK(storm::from_json_paths("{\"paths\":[\"/foo/.///bar/../\"]}").at(0).path.lexically_normal() == "/foo/");
  CHECK_THROWS_AS(storm::from_json_paths("{ }"), std::exception);
  CHECK_THROWS_AS(storm::from_json_paths(""), std::exception);  
}

#else

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

#endif