#include "stage_request.hpp"
#include "file.hpp"
#include "types.hpp"

#include <doctest.h>
#include <ctime>

TEST_SUITE_BEGIN("StageRequest");
TEST_CASE("Update timestamps")
{
  // Sunday, 1 January 2023 00:00:00
  const storm::TimePoint now = 1672531200;

  // clang-format off
  storm::Files only_submitted_files {
    {"/tmp/foo", "/root/tmp/foo", storm::File::State::submitted},
    {"/tmp/bar", "/root/tmp/bar", storm::File::State::submitted},
    {"/tmp/baz", "/root/tmp/baz", storm::File::State::submitted}
  };
  // clang-format on

  storm::StageRequest not_started{only_submitted_files};
  not_started.update_timestamps();
  CHECK_EQ(not_started.started_at, 0);
  CHECK_EQ(not_started.completed_at, 0);

  // clang-format off
  storm::Files unfinished_files {
    {"/tmp/zab", "/root/tmp/zab", storm::File::State::submitted, storm::Locality::unavailable, 0,        0       },
    {"/tmp/bof", "/root/tmp/bof", storm::File::State::submitted, storm::Locality::unavailable, 0,        0       },
    {"/tmp/rab", "/root/tmp/rab", storm::File::State::started,   storm::Locality::unavailable, now + 5,  0       },
    {"/tmp/fob", "/root/tmp/fob", storm::File::State::cancelled, storm::Locality::unavailable, now + 15, now + 35},
    {"/tmp/foo", "/root/tmp/foo", storm::File::State::failed,    storm::Locality::unavailable, now,      now + 10},
    {"/tmp/baz", "/root/tmp/baz", storm::File::State::failed,    storm::Locality::unavailable, now,      now + 20},
    {"/tmp/bar", "/root/tmp/bar", storm::File::State::completed, storm::Locality::unavailable, now + 1,  now + 20},
    {"/tmp/oof", "/root/tmp/oof", storm::File::State::completed, storm::Locality::unavailable, now + 10, now + 20}
  };
  // clang-format on

  storm::StageRequest unfinished_stage{unfinished_files};
  unfinished_stage.update_timestamps();
  CHECK_EQ(unfinished_stage.started_at, now);
  CHECK_EQ(unfinished_stage.completed_at, 0);

  // clang-format off
  storm::Files finished_files {
    {"/tmp/fob", "/root/tmp/fob", storm::File::State::cancelled, storm::Locality::unavailable, now + 15, now + 35},
    {"/tmp/foo", "/root/tmp/foo", storm::File::State::failed,    storm::Locality::unavailable, now,      now + 10},
    {"/tmp/baz", "/root/tmp/baz", storm::File::State::failed,    storm::Locality::unavailable, now,      now + 20},
    {"/tmp/zab", "/root/tmp/zab", storm::File::State::failed,    storm::Locality::unavailable, now + 25, now + 45},
    {"/tmp/bof", "/root/tmp/bof", storm::File::State::failed,    storm::Locality::unavailable, now + 25, now + 55},
    {"/tmp/bar", "/root/tmp/bar", storm::File::State::completed, storm::Locality::unavailable, now + 1,  now + 20},
    {"/tmp/oof", "/root/tmp/oof", storm::File::State::completed, storm::Locality::unavailable, now + 10, now + 20},
    {"/tmp/rab", "/root/tmp/rab", storm::File::State::completed, storm::Locality::unavailable, now + 5,  now + 15}
  };
  // clang-format on

  storm::StageRequest finished_stage{finished_files};
  finished_stage.update_timestamps();

  CHECK_EQ(finished_stage.started_at, now);
  CHECK_EQ(finished_stage.completed_at, now + 55);


  // clang-format off
  storm::Files completed_files {
    {"/tmp/foo", "/root/tmp/foo", storm::File::State::completed, storm::Locality::disk,          now,      now + 20},
    {"/tmp/bar", "/root/tmp/bar", storm::File::State::completed, storm::Locality::disk_and_tape, now + 10, now + 50},
    {"/tmp/baz", "/root/tmp/baz", storm::File::State::completed, storm::Locality::disk,          now + 5,  now + 35}
  };
  // clang-format on

  storm::StageRequest complated_stage{completed_files};
  complated_stage.update_timestamps();

  CHECK_EQ(complated_stage.started_at, now);
  CHECK_EQ(complated_stage.completed_at, now + 50);

  // clang-format off
  storm::Files cancelled_files {
    {"/tmp/foo", "/root/tmp/foo", storm::File::State::cancelled, storm::Locality::tape, now,      now + 20},
    {"/tmp/bar", "/root/tmp/bar", storm::File::State::cancelled, storm::Locality::tape, now + 10, now + 60},
    {"/tmp/baz", "/root/tmp/baz", storm::File::State::cancelled, storm::Locality::tape, now + 5,  now + 35}
  };
  // clang-format on

  storm::StageRequest cancelled_stage{cancelled_files};
  cancelled_stage.update_timestamps();

  CHECK_EQ(cancelled_stage.started_at, now);
  CHECK_EQ(cancelled_stage.completed_at, now + 60);

  // clang-format off
  storm::Files failed_files {
    {"/tmp/foo", "/root/tmp/foo", storm::File::State::failed, storm::Locality::tape, now,      now + 20},
    {"/tmp/bar", "/root/tmp/bar", storm::File::State::failed, storm::Locality::lost, now + 10, now + 40},
    {"/tmp/baz", "/root/tmp/baz", storm::File::State::failed, storm::Locality::lost, now + 5,  now + 35}
  };
  // clang-format on

  storm::StageRequest failed_stage{failed_files};
  failed_stage.update_timestamps();

  CHECK_EQ(failed_stage.started_at, now);
  CHECK_EQ(failed_stage.completed_at, now + 40);
}

TEST_SUITE_END;