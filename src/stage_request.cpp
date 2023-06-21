#include "stage_request.hpp"
#include "io.hpp"
#include <algorithm>
#include <cassert>

namespace storm {
/// Update the StageRequest's started_at and finished_at timestamps.
/// started_at and finished_at are calculated as the mininum started_at time of
/// all files in a state different than submitted, and the maximum finished_at
/// time of all files in a final state (finished, failed, cancelled),
/// respectively.
bool StageRequest::update_timestamps()
{
  assert(std::is_sorted(
      files.begin(), files.end(), [](auto const& f1, auto const& f2) {
        return to_underlying(f1.state) < to_underlying(f2.state);
      }));

  bool updated = false;

  if (started_at == 0 && files.back().state != File::State::submitted) {
    auto const it =
        std::partition_point(files.begin(), files.end(), [](File const& file) {
          return file.state == File::State::submitted;
        });

    auto const first_started_file =
        std::min_element(it, files.end(), [](const File& f1, const File& f2) {
          assert(f1.started_at > 0 && f2.started_at > 0);
          return f1.started_at < f2.started_at;
        });
    started_at = first_started_file->started_at;
    updated    = true;
  }

  if (completed_at == 0 && is_final(files.front().state)) {
    const auto latest_file = std::max_element(
        files.begin(), files.end(), [](const File& f1, const File& f2) {
          return f1.finished_at < f2.finished_at;
        });
    completed_at = latest_file->finished_at;
    updated      = true;
  }
  return updated;
}
} // namespace storm
