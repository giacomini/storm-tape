#include "configuration.hpp"
#include "database.hpp"
#include "database_soci.hpp"
#include "routes.hpp"
#include "tape_service.hpp"

#ifdef ENABLE_TESTING

#  define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#  include "archive_response.hpp"
#  include "cancel_response.hpp"
#  include "fs_storage_test.hpp"
#  include "io.hpp"
#  include "release_response.hpp"
#  include "requests_with_paths.hpp"
#  include "stage_response.hpp"
#  include "../test/doctest.h"
#  include <cstdio>

// clang-format off
TEST_CASE("Test IO")
{
  storm::FsStorageTest::SetUp();
  //IO for stage requests
  CHECK(storm::from_json(R"({"files":[{"path":"/tmp/foo/bar.txt"}]})").size() == 1); //OK, 201
  CHECK(storm::from_json(R"({"files":[{"path":"/tmp/foo/bar.txt"},{"path":"/tmp/bar/foo.txt"}]})").size() == 2); //OK, 201
  CHECK(storm::from_json(R"({"files":[{"path":"/tmp/foo//bar.txt"}]})").at(0).path.lexically_normal() == "/tmp/foo/bar.txt"); //sanitise path
  CHECK(storm::from_json(R"({"files":[{"path":"/tmp/foo/.///bar/../"}]})").at(0).path.lexically_normal() == "/tmp/foo/"); //sanitise path
  CHECK_THROWS_AS(storm::from_json("{ }"), std::exception); //invalid stage request, 400
  CHECK_THROWS_AS(storm::from_json(""), std::exception); //invalid stage request, 400
  //IO for cancel, release, and archive info requests
  CHECK(storm::from_json_paths(R"({"paths":["/tmp/foo/bar.txt"]})").size() == 1); //OK
  CHECK(storm::from_json_paths(R"({"paths":["/tmp/foo/bar.txt","/tmp/bar/foo.txt"]})").size() == 2); //OK
  CHECK(storm::from_json_paths(R"({"paths":["/tmp/foo//bar.txt"]})").at(0).path.lexically_normal() == "/tmp/foo/bar.txt"); //sanitise path
  CHECK(storm::from_json_paths(R"({"paths":["/tmp/foo/.///bar/../"]})").at(0).path.lexically_normal() == "/tmp/foo/"); //sanitise path
  CHECK_THROWS_AS(storm::from_json_paths("{ }"), std::exception); //invalid cancel/release/archive request, 400
  CHECK_THROWS_AS(storm::from_json_paths(""), std::exception); //invalid cancel/release/archive request, 400
}

TEST_CASE("Test with cancel request")
{
  if ( std::remove("storm-tape-test.db"))
std::cout << "Removed\n";
  soci::session sql(soci::sqlite3, "storm-tape-test.db");
  storm::SociDatabase db{sql};
  storm::TapeService service{db};
  auto files = storm::from_json(R"({"files":[{"path":"/tmp/foo/bar.txt"}]})");
  storm::StageRequest request{files};
  auto stage_resp = service.stage(request);
  storm::CancelRequest cancel{R"({"paths":["/tmp/not/foo/bar.txt"]})"};
  storm::CancelRequest cancel2{R"({"paths":["/tmp/foo/bar.txt"]})"};
  storm::CancelRequest cancel3{R"({"paths":["/tmp/foo/bar.txt","/tmp/not/foo/bar.txt"]})"};
  auto cancel_resp  = service.cancel(stage_resp.id(), cancel);
  auto cancel_resp2 = service.cancel(stage_resp.id(), cancel2);
  auto cancel_resp3 = service.cancel(stage_resp.id(), cancel3);
  // Invalid determines the files in the CancelRequest not in StageRequest.
  CHECK(cancel_resp.invalid().size() == 1); // Asking to cancel path not in stage, 400
  CHECK(cancel_resp2.invalid().size() == 0); //Asking to cancel path in stage, OK 200
  CHECK(cancel_resp3.invalid().size() == 1); // Asking to cancel 2 path, with one not in stage, 400
  std::remove("storm-tape-test.db");
}

TEST_CASE("Test when release request")
{
  soci::session sql(soci::sqlite3, "storm-tape-test.db");
  storm::SociDatabase db{sql};
  storm::TapeService service{db};
  auto files = storm::from_json(R"({"files":[{"path":"/tmp/foo/bar.txt"}]})");
  storm::StageRequest request{files};
  auto stage_resp = service.stage(request);
  storm::ReleaseRequest release{R"({"paths":["/tmp/not/foo/bar.txt"]})"};
  storm::ReleaseRequest release2{R"({"paths":["/tmp/foo/bar.txt"]})"};
  storm::ReleaseRequest release3{R"({"paths":["/tmp/foo/bar.txt","/tmp/not/foo/bar.txt"]})"};
  auto release_resp  = service.release(stage_resp.id(), release);
  auto release_resp2 = service.release(stage_resp.id(), release2);
  auto release_resp3 = service.release(stage_resp.id(), release3);
  // Invalid determines the files in the ReleaseRequest, not in StageRequest.
  CHECK(release_resp.invalid().size() == 1); // Asking to release path not in stage, bad request 400
  CHECK(release_resp2.invalid().size() == 0); // Asking to release path in stage, OK 200
  CHECK(release_resp3.invalid().size() == 1); // Asking to release 2 path, with one not in stage, bad request 400
  std::remove("storm-tape-test.db");
}

TEST_CASE("Test with archive info")
{
  soci::session sql(soci::sqlite3, "storm-tape-test.db");
  storm::SociDatabase db{sql};
  storm::TapeService service{db};
  auto files = storm::from_json(R"({"files":[{"path":"/tmp/foo/bar.txt"}]})");
  storm::StageRequest request{files};
  service.stage(request);
  storm::ArchiveInfo info{R"({"paths":["/tmp/not/foo/bar.txt"]})"};
  storm::ArchiveInfo info2{R"({"paths":["/tmp/foo/bar.txt"]})"};
  storm::ArchiveInfo info3{R"({"paths":["/tmp/foo/bar.txt","/tmp/not/foo/bar.txt"]})"};
  auto info_resp  = service.archive(info);
  auto info_resp2 = service.archive(info2);
  auto info_resp3 = service.archive(info3);
  // Invalid determines the files in the ArchiveInfo request, not in StageRequest. OK 200, error in body message.
  // Valid determines the files in the ArchiveInfo request, and in StageRequest. OK 200, shows locality and path in body.
  //
  // One archive info, not in stage
  CHECK(info_resp.invalid().size() == 1);
  CHECK(info_resp.valid().size() == 0);
  // One archive info, in stage
  CHECK(info_resp2.invalid().size() == 0);
  CHECK(info_resp2.valid().size() == 1);
  // Two archive info, one in stage and one not
  CHECK(info_resp3.invalid().size() == 1);
  CHECK(info_resp3.valid().size() == 1);

  auto files2 = storm::from_json(R"({"files":[{"path":"/tmp/not/foo/bar.txt"}]})");
  storm::StageRequest request2{files2};
  service.stage(request2);
  storm::ArchiveInfo info4{R"({"paths":["/tmp/foo/bar.txt","/tmp/not/foo/bar.txt"]})"};
  auto info_resp4 = service.archive(info4);
  // Adding new stage request. Same archive info as before, now all two in stage
  CHECK(info_resp4.invalid().size() == 0);
  CHECK(info_resp4.valid().size() == 2);

  storm::FsStorageTest::TearDown();
  std::remove("storm-tape-test.db");
}
// clang-format on
#else

int main(int, char*[])
{
  soci::session sql(soci::sqlite3, "storm-tape.db");
  crow::SimpleApp app;
  storm::SociDatabase db{sql};
  storm::Configuration config{};
  storm::TapeService service{db};

  storm::create_routes(app, config, service);
  app.port(config.port).run();
}

#endif