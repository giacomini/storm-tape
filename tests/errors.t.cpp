#include "errors.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "delete_response.hpp"
#include "fixture.t.hpp"
#include "io.hpp"
#include "release_response.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "tape_service.hpp"
#include <doctest.h>

namespace storm {

static const StageId invalid_id{"123-abcd-666"};
static StageId make_stage(TapeService& service)
{
  StageRequest req{{File{"test", "test"}}, 0, 0, 0};
  return service.stage(req).id();
}

TEST_SUITE_BEGIN("Errors");
TEST_CASE_FIXTURE(TestFixture, "Stage")
{
  {
    auto json =
        R"({"files":[{"path":"/tmp//example.txt"},{"path":"/tmp/example2.txt"}]})";
    CHECK_NOTHROW(from_json(json, StageRequest::tag));
  }
  {
    auto json =
        R"({"files"[{"path":"/tmp//example.txt"},{"path":"/tmp/example2.txt"}]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, StageRequest::tag), BadRequest,
                            "JSON validation error");
  }
  {
    auto json =
        R"(({"FiLeS":[{"path":"/tmp//example.txt"},{"path":"/tmp/example2.txt"}]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, StageRequest::tag), BadRequest,
                            "JSON validation error");
  }
}

TEST_CASE_FIXTURE(TestFixture, "Status")
{
  auto const valid_id = make_stage(m_service);
  CHECK_THROWS_AS_MESSAGE(m_service.status(invalid_id), StageNotFound,
                          "Stage Not Found");
  CHECK_NOTHROW(m_service.status(valid_id));
}

TEST_CASE_FIXTURE(TestFixture, "Cancel")
{
  {
    auto json = R"({"paths":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_NOTHROW(from_json(json, CancelRequest::tag));
  }
  {
    auto json = R"({"files":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, CancelRequest::tag), BadRequest,
                            "JSON validation error");
  }
  {
    auto json = R"({"paths":["/tmp/example.txt""/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, CancelRequest::tag), BadRequest,
                            "JSON validation error");
  }
  auto const valid_id = make_stage(m_service);
  CHECK_NOTHROW(m_service.cancel(valid_id, CancelRequest{}));
  CHECK_THROWS_AS_MESSAGE(m_service.cancel(invalid_id, CancelRequest{}),
                          StageNotFound, "Stage Not Found");
}

TEST_CASE_FIXTURE(TestFixture, "Delete")
{
  auto const valid_id = make_stage(m_service);
  CHECK_THROWS_AS_MESSAGE(m_service.erase(invalid_id), StageNotFound,
                          "Stage Not Found");
  CHECK_NOTHROW(m_service.erase(valid_id));
}

TEST_CASE_FIXTURE(TestFixture, "Release")
{
  {
    auto json = R"({"paths":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_NOTHROW(from_json(json, ReleaseRequest::tag));
  }
  {
    auto json = R"({"files":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, ReleaseRequest::tag), BadRequest,
                            "JSON validation error");
  }
  {
    auto json = R"({"paths":["/tmp/example.txt""/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, ReleaseRequest::tag), BadRequest,
                            "JSON validation error");
  }
  auto const valid_id = make_stage(m_service);
  CHECK_THROWS_AS_MESSAGE(m_service.release(invalid_id, {}), StageNotFound,
                          "Stage Not Found");
  CHECK_NOTHROW(m_service.release(valid_id, {}));
}

TEST_CASE_FIXTURE(TestFixture, "ArchiveInfo")
{
  {
    auto json = R"({"paths":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_NOTHROW(from_json(json, ArchiveInfoRequest::tag));
  }
  {
    auto json = R"({"files":["/tmp/example.txt","/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, ArchiveInfoRequest::tag),
                            BadRequest, "JSON validation error");
  }
  {
    auto json = R"({"paths":["/tmp/example.txt""/tmp/example2.txt"]})";
    CHECK_THROWS_AS_MESSAGE(from_json(json, ArchiveInfoRequest::tag),
                            BadRequest, "JSON validation error");
  }
}

TEST_CASE_FIXTURE(TestFixture, "TakeOver")
{
  {
    auto query_string = "first=123";
    CHECK_NOTHROW(from_body_params(query_string, TakeOverRequest::tag));
  }
  {
    auto query_string = "second=123";
    CHECK_THROWS_AS_MESSAGE(from_json(query_string, ArchiveInfoRequest::tag),
                            BadRequest, "Query Parameters validation error");
  }
  {
    auto query_string = "first=12.3";
    CHECK_THROWS_AS_MESSAGE(from_json(query_string, ArchiveInfoRequest::tag),
                            BadRequest, "Query Parameters validation error");
  }
  {
    auto query_string = "first=p123";
    CHECK_THROWS_AS_MESSAGE(from_json(query_string, ArchiveInfoRequest::tag),
                            BadRequest, "Query Parameters validation error");
  }
  {
    auto query_string = "first=123x";
    CHECK_THROWS_AS_MESSAGE(from_json(query_string, ArchiveInfoRequest::tag),
                            BadRequest, "Query Parameters validation error");
  }
}

} // namespace storm