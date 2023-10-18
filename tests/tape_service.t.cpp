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
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include "types.hpp"

namespace storm {

auto make_file = [](Path const& path, const char* size = "1M") {
  auto const cmd = fmt::format(
      "dd if=/dev/random bs={} count=1 of={} &> /dev/null", size, path.c_str());
  std::system(cmd.c_str());
  set_xattr(path, XAttrName{"user.storm.migrated"}, XAttrValue{""});
};

auto make_stub = [](Path const& path, const char* size = "1M") {
  auto const cmd = fmt::format(
      "dd if=/dev/zero conv=sparse bs={} count=1 of={} &> /dev/null", size,
      path.c_str());
  std::system(cmd.c_str());
  set_xattr(path, XAttrName{"user.storm.migrated"}, XAttrValue{""});
};

auto delete_file = [](Path const& path) { std::filesystem::remove(path); };

TEST_SUITE_BEGIN("TapeService");

auto const now = std::time(nullptr);
const Files FILES{{"/tmp/example1.txt", "/tmp/example1.txt"},
                  {"/tmp/example2.txt", "/tmp/example2.txt"}};

TEST_CASE_FIXTURE(TestFixture, "Stage")
{
  StageRequest request{FILES, now, 0, 0};
  make_stub(request.files.at(0).physical_path);
  make_file(request.files.at(1).physical_path);

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

    CHECK_EQ(files.at(0).physical_path, "/tmp/example1.txt");
    CHECK_EQ(files.at(0).state, File::State::submitted);
    CHECK_EQ(files.at(0).started_at, 0);
    CHECK_EQ(files.at(0).finished_at, 0);
    CHECK_EQ(files.at(1).physical_path, "/tmp/example2.txt");
    CHECK_EQ(files.at(1).state, File::State::completed);
    CHECK_GT(files.at(1).started_at, 0);
    CHECK_EQ(files.at(1).finished_at, files.at(1).started_at);

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

    CHECK_EQ(files.at(0).physical_path, "/tmp/example1.txt");
    CHECK_EQ(files.at(0).state, File::State::submitted);
    CHECK_EQ(files.at(0).started_at, 0);
    CHECK_EQ(files.at(0).finished_at, 0);
    CHECK_EQ(files.at(1).physical_path, "/tmp/example2.txt");
    CHECK_EQ(files.at(1).state, File::State::completed);
    CHECK_GT(files.at(1).started_at, 0);
    CHECK_EQ(files.at(1).finished_at, files.at(1).started_at);
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
    CHECK(has_xattr(Path{"/tmp/example1.txt"}, XAttrName{"user.TSMRecT"}));
    CHECK_FALSE(
        has_xattr(Path{"/tmp/example2.txt"}, XAttrName{"user.TSMRecT"}));
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
    CHECK_EQ(files.at(0).physical_path, "/tmp/example1.txt");
    CHECK_EQ(files.at(0).state, File::State::started);
    CHECK_GE(files.at(0).started_at, 0);
    CHECK_EQ(files.at(0).finished_at, 0);

    CHECK_EQ(files.at(1).state, File::State::completed);
    CHECK_EQ(files.at(1).physical_path, "/tmp/example2.txt");
    CHECK_GE(files.at(1).started_at, files.at(1).finished_at);
    CHECK_GE(files.at(1).finished_at, files.at(1).started_at);
  }
  {
    // check from db
    auto maybe_stage = m_db.find(id);
    CHECK(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    auto& files = stage.files;
    CHECK_EQ(files.at(0).state, File::State::started);
    CHECK_EQ(files.at(0).physical_path, "/tmp/example1.txt");
    CHECK_GE(files.at(0).started_at, 0);
    CHECK_EQ(files.at(0).finished_at, 0);

    CHECK_EQ(files.at(1).state, File::State::completed);
    CHECK_EQ(files.at(1).physical_path, "/tmp/example2.txt");
    CHECK_GT(files.at(1).started_at, 0);
    CHECK_GT(files.at(1).finished_at, 0);
    CHECK_GE(files.at(1).finished_at, files.at(1).started_at);
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
    CHECK_EQ(files.at(0).physical_path, "/tmp/example1.txt");
    CHECK_EQ(files.at(0).state, File::State::completed);
    CHECK_GT(files.at(0).finished_at, files.at(0).started_at);
    CHECK_GT(files.at(0).started_at, files.at(1).started_at);

    CHECK_EQ(files.at(1).physical_path, "/tmp/example2.txt");
    CHECK_EQ(files.at(1).state, File::State::completed);
    CHECK_EQ(files.at(1).started_at, files.at(1).finished_at);

    CHECK_GE(stage.started_at, stage.created_at);
    CHECK_GT(stage.completed_at, stage.created_at);
    CHECK_EQ(stage.started_at, files.at(1).started_at);
    CHECK_EQ(stage.completed_at, files.at(0).finished_at);
  }
  {
    // check from db
    auto maybe_stage = m_db.find(id);
    CHECK(maybe_stage.has_value());
    auto& stage = maybe_stage.value();
    auto& files = stage.files;
    CHECK_EQ(files.at(0).state, File::State::completed);
    CHECK_EQ(files.at(0).physical_path, "/tmp/example1.txt");
    CHECK_GT(files.at(0).started_at, 0);
    CHECK_GE(files.at(0).finished_at, files.at(0).started_at);

    CHECK_EQ(files.at(1).state, File::State::completed);
    CHECK_EQ(files.at(1).physical_path, "/tmp/example2.txt");
    CHECK_GT(files.at(1).started_at, 0);
    CHECK_GT(files.at(1).finished_at, 0);
    CHECK_GE(files.at(1).finished_at, files.at(1).started_at);
    CHECK_GE(stage.started_at, stage.created_at);
    CHECK_GT(stage.completed_at, stage.created_at);
    CHECK_EQ(stage.started_at, files.at(1).started_at);
    CHECK_EQ(stage.completed_at, files.at(0).finished_at);
  }

  for (auto& f : request.files) {
    delete_file(f.physical_path);
  }
}

TEST_CASE_FIXTURE(TestFixture, "Cancel")
{
  StageRequest request{FILES, now, 0, 0};
  make_stub(request.files.at(0).physical_path);
  make_file(request.files.at(1).physical_path);
  // Do stage
  auto stage_response = m_service.stage(std::move(request));
  auto id             = stage_response.id();
  // Do cancel
  Paths paths;
  std::transform(FILES.begin(), FILES.end(), std::back_inserter(paths),
                 [](File const& f) { return LogicalPath{f.logical_path}; });
  m_service.cancel(id, CancelRequest{paths});
  // Sleep for a while...
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);
  // Do status
  auto stage = m_service.status(id).stage();
  CHECK_EQ(stage.created_at, now);
  CHECK_GT(stage.started_at, now);
  CHECK_EQ(stage.started_at, stage.completed_at);
  CHECK(std::all_of(stage.files.begin(), stage.files.end(),
              [](auto& f) { return f.state == File::State::cancelled; }));
}
} // namespace storm