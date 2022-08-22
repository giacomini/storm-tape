#ifndef STAGE_REQUEST_HPP
#define STAGE_REQUEST_HPP

#include "file.hpp"
#include <vector>

namespace storm { // FG should a path be absolute? can there be duplicates?
struct StageRequest
{
 private:
  std::vector<File> m_files;

 public:
  using Clock = std::chrono::system_clock;

  Clock::time_point created_at;
  Clock::time_point started_at;
  std::vector<File> files;

  StageRequest(std::vector<File> files);
  std::vector<File> getFiles() const;
  void setFiles(std::vector<File> fle);
};
} // namespace storm

#endif