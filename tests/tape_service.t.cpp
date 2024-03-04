#include <doctest.h>
#include <fmt/core.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "cancel_response.hpp"
#include "extended_attributes.hpp"
#include "file.hpp"
#include "fixture.t.hpp"
#include "in_progress_request.hpp"
#include "in_progress_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include "types.hpp"

namespace storm {

auto make_file = [](PhysicalPath const& path, const char* size = "1M") {
  auto const cmd = fmt::format(
      "dd if=/dev/random bs={} count=1 of={} &> /dev/null", size, path.c_str());
  std::system(cmd.c_str());
  set_xattr(path, XAttrName{"user.storm.migrated"}, XAttrValue{""});
};

auto make_stub = [](PhysicalPath const& path, const char* size = "1M") {
  auto const cmd = fmt::format(
      "dd if=/dev/zero conv=sparse bs={} count=1 of={} &> /dev/null", size,
      path.c_str());
  std::system(cmd.c_str());
  set_xattr(path, XAttrName{"user.storm.migrated"}, XAttrValue{""});
};

auto delete_file = [](PhysicalPath const& path) {
  std::filesystem::remove(path);
};

TEST_SUITE_BEGIN("TapeService");

auto const now = std::time(nullptr);
const Files FILES{{"/tmp/example1.txt", "/tmp/example1.txt"},
                  {"/tmp/example2.txt", "/tmp/example2.txt"}};

TEST_CASE_FIXTURE(TestFixture, "Stage")
{
  StageRequest request{FILES, now, 0, 0};
  REQUIRE_GE(request.files.size(), 2);
  make_stub(request.files[0].physical_path);
  make_file(request.files[1].physical_path);

  // Do stage
  auto stage_response = m_service.stage(std::move(request));
  auto id             = stage_response.id();

  {
    auto status_response = m_service.status(id);
    auto& stage          = status_response.stage();
    auto& files          = stage.files;

    CHECK(files[0].state == File::State::submitted);
    CHECK(files[1].state == File::State::completed);
  }

  {
    fs::remove(FILES[0].physical_path);
    fs::remove(FILES[1].physical_path);

    auto status_response = m_service.status(id);
    auto& stage          = status_response.stage();
    auto& files          = stage.files;

    CHECK(files[0].state == File::State::failed);
    CHECK(files[1].state == File::State::completed);
  }
}

TEST_CASE_FIXTURE(TestFixture, "In Progress")
{
  StageRequest request{FILES, now, 0, 0};
  REQUIRE_GE(request.files.size(), 2);
  make_stub(request.files[0].physical_path);
  make_stub(request.files[1].physical_path);

  // Do stage
  auto stage_response = m_service.stage(std::move(request));
  auto id             = stage_response.id();

  {
    auto const resp = m_service.in_progress({.precise = 0});
    CHECK(resp.paths.empty());
  }

  {
    auto const resp = m_service.in_progress({.precise = 1});
    CHECK(resp.paths.empty());
  }

  {
    auto const to_resp = m_service.take_over({.n_files = 10});

    CHECK_EQ(to_resp.paths.size(), 2);
    {
      auto const resp = m_service.in_progress({.precise = 0});
      CHECK_EQ(resp.paths.size(), 2);
    }
    {
      auto const resp = m_service.in_progress({.precise = 1});
      CHECK_EQ(resp.paths.size(), 2);
    }

    {
      // Simulate the end of the recall
      make_file(FILES[0].physical_path);
      storm::remove_xattr(FILES[0].physical_path, XAttrName{"user.TSMRecT"});
    }
    {
      auto const resp = m_service.in_progress({.precise = 0});
      CHECK_EQ(resp.paths.size(), 2);
    }
    {
      auto const resp = m_service.in_progress({.precise = 1});
      CHECK_EQ(resp.paths.size(), 1);
    }

    auto status_response = m_service.status(id);
    auto& files          = status_response.stage().files;

    CHECK_EQ(files.size(), 2);
    CHECK(std::all_of(files.begin(), files.end(), [](auto const& f) {
      if (f.physical_path == FILES[0].physical_path) {
        return f.state == File::State::completed;
      } else if (f.physical_path == FILES[1].physical_path) {
        return f.state == File::State::started;
      } else {
        return false;
      }
    }));
  }

  {
    auto const resp = m_service.in_progress({.precise = 0});
    CHECK_EQ(resp.paths.size(), 1);
  }

  {
    auto const resp = m_service.in_progress({.precise = 1});
    CHECK_EQ(resp.paths.size(), 1);
  }

  for (auto& f : FILES) {
    delete_file(f.physical_path);
  }
}

TEST_CASE_FIXTURE(TestFixture, "Stage")
{
  StageRequest request{FILES, now, 0, 0};
  REQUIRE_GE(request.files.size(), 2);
  make_stub(request.files[0].physical_path);
  make_file(request.files[1].physical_path);

  // Do stage
  auto stage_response = m_service.stage(std::move(request));
  auto id             = stage_response.id();
  {
    auto maybe_stage = m_db.find(id);

    CHECK(maybe_stage.has_value());

    auto& stage = maybe_stage.value();
    auto& files = stage.files;

    // clang-format off
    CHECK(std::all_of(files.begin(), files.end(), [](auto const& f) {
      return f.state == File::State::submitted;
    }));

    CHECK(std::all_of(files.begin(), files.end(), [](auto const& f) {
      return f.started_at == f.finished_at && f.started_at == 0;
    }));
    // clang-format on
  }

  // Do status
  {
    // check from response
    auto status_response = m_service.status(id);
    auto& stage          = status_response.stage();
    auto& files          = stage.files;

    REQUIRE_GE(files.size(), 2);
    CHECK_EQ(files[0].physical_path, "/tmp/example1.txt");
    CHECK_EQ(files[0].state, File::State::submitted);
    CHECK_EQ(files[0].started_at, 0);
    CHECK_EQ(files[0].finished_at, 0);
    CHECK_EQ(files[1].physical_path, "/tmp/example2.txt");
    CHECK_EQ(files[1].state, File::State::completed);
    CHECK_GT(files[1].started_at, 0);
    CHECK_EQ(files[1].finished_at, files[1].started_at);

    CHECK_EQ(stage.created_at, now);
    CHECK_GE(stage.started_at, now);
    CHECK_EQ(stage.completed_at, 0);
  }
  {
    // check from db
    auto maybe_stage = m_db.find(id);
    CHECK(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    auto& files = stage.files;

    REQUIRE_GE(files.size(), 2);
    CHECK_EQ(files[0].physical_path, "/tmp/example1.txt");
    CHECK_EQ(files[0].state, File::State::submitted);
    CHECK_EQ(files[0].started_at, 0);
    CHECK_EQ(files[0].finished_at, 0);
    CHECK_EQ(files[1].physical_path, "/tmp/example2.txt");
    CHECK_EQ(files[1].state, File::State::completed);
    CHECK_GT(files[1].started_at, 0);
    CHECK_EQ(files[1].finished_at, files[1].started_at);
    CHECK_EQ(stage.created_at, now);
    CHECK_GE(stage.started_at, now);
    CHECK_EQ(stage.completed_at, 0);
  }

  // Sleep for a while...
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  // Do take over
  {
    // Since one file was already on disk, only the stub file should be returned
    auto const takeover_response = m_service.take_over({42});
    CHECK_EQ(takeover_response.paths.size(), 1);
    CHECK(has_xattr(PhysicalPath{"/tmp/example1.txt"},
                    XAttrName{"user.TSMRecT"}));
    CHECK_FALSE(has_xattr(PhysicalPath{"/tmp/example2.txt"},
                          XAttrName{"user.TSMRecT"}));
  }

  // Sleep for a while...
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  // Do status again
  {
    // check from response
    auto const status = m_service.status(id);
    auto& stage       = status.stage();
    auto& files       = stage.files;
    CHECK_EQ(status.id(), id);
    REQUIRE_GE(files.size(), 2);
    CHECK_EQ(files[0].physical_path, "/tmp/example1.txt");
    CHECK_EQ(files[0].state, File::State::started);
    CHECK_GE(files[0].started_at, 0);
    CHECK_EQ(files[0].finished_at, 0);

    CHECK_EQ(files[1].state, File::State::completed);
    CHECK_EQ(files[1].physical_path, "/tmp/example2.txt");
    CHECK_GE(files[1].started_at, files[1].finished_at);
    CHECK_GE(files[1].finished_at, files[1].started_at);
  }
  {
    // check from db
    auto maybe_stage = m_db.find(id);
    CHECK(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    auto& files = stage.files;
    REQUIRE_GE(files.size(), 2);
    CHECK_EQ(files[0].state, File::State::started);
    CHECK_EQ(files[0].physical_path, "/tmp/example1.txt");
    CHECK_GE(files[0].started_at, 0);
    CHECK_EQ(files[0].finished_at, 0);

    CHECK_EQ(files[1].state, File::State::completed);
    CHECK_EQ(files[1].physical_path, "/tmp/example2.txt");
    CHECK_GT(files[1].started_at, 0);
    CHECK_GT(files[1].finished_at, 0);
    CHECK_GE(files[1].finished_at, files[1].started_at);
  }
  // Simulate the end of the recall
  {
    make_file("/tmp/example1.txt");
    remove_xattr("/tmp/example1.txt", XAttrName{"user.TSMRecT"});
  }

  // Do status for the last time, now everything should be finished
  {
    // check from response
    auto const status = m_service.status(id);
    auto& stage       = status.stage();
    auto& files       = stage.files;
    CHECK_EQ(status.id(), id);
    REQUIRE_GE(files.size(), 2);
    CHECK_EQ(files[0].physical_path, "/tmp/example1.txt");
    CHECK_EQ(files[0].state, File::State::completed);
    CHECK_GT(files[0].finished_at, files[0].started_at);
    CHECK_GT(files[0].started_at, files[1].started_at);

    CHECK_EQ(files[1].physical_path, "/tmp/example2.txt");
    CHECK_EQ(files[1].state, File::State::completed);
    CHECK_EQ(files[1].started_at, files[1].finished_at);

    CHECK_GE(stage.started_at, stage.created_at);
    CHECK_GT(stage.completed_at, stage.created_at);
    CHECK_EQ(stage.started_at, files[1].started_at);
    CHECK_EQ(stage.completed_at, files[0].finished_at);
  }
  {
    // check from db
    auto maybe_stage = m_db.find(id);
    CHECK(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    auto& files = stage.files;
    REQUIRE_GE(files.size(), 2);
    CHECK_EQ(files[0].state, File::State::completed);
    CHECK_EQ(files[0].physical_path, "/tmp/example1.txt");
    CHECK_GT(files[0].started_at, 0);
    CHECK_GE(files[0].finished_at, files[0].started_at);

    CHECK_EQ(files[1].state, File::State::completed);
    CHECK_EQ(files[1].physical_path, "/tmp/example2.txt");
    CHECK_GT(files[1].started_at, 0);
    CHECK_GT(files[1].finished_at, 0);
    CHECK_GE(files[1].finished_at, files[1].started_at);
    CHECK_GE(stage.started_at, stage.created_at);
    CHECK_GT(stage.completed_at, stage.created_at);
    CHECK_EQ(stage.started_at, files[1].started_at);
    CHECK_EQ(stage.completed_at, files[0].finished_at);
  }

  for (auto& f : request.files) {
    delete_file(f.physical_path);
  }
}

TEST_CASE_FIXTURE(TestFixture, "Cancel")
{
  StageRequest request{FILES, now, 0, 0};
  REQUIRE_GE(request.files.size(), 2);
  make_stub(request.files[0].physical_path);
  make_file(request.files[1].physical_path);
  // Do stage
  auto stage_response = m_service.stage(std::move(request));
  auto& id            = stage_response.id();
  // Do cancel
  LogicalPaths paths;
  std::transform(FILES.begin(), FILES.end(), std::back_inserter(paths),
                 [](File const& f) { return LogicalPath{f.logical_path}; });
  m_service.cancel(id, CancelRequest{paths});
  // Do status
  auto stage = m_service.status(id).stage();
  CHECK_EQ(stage.created_at, now);
  CHECK_GT(stage.started_at, now);
  CHECK_EQ(stage.started_at, stage.completed_at);
  CHECK(std::all_of(stage.files.begin(), stage.files.end(),
                    [](auto& f) { return f.state == File::State::cancelled; }));
}
} // namespace storm