#ifndef STAGE_REQUEST_HPP
#define STAGE_REQUEST_HPP

#include "file.hpp"
#include <vector>

namespace storm {
class StageRequest
{
  std::chrono::system_clock::time_point m_created_at;
  std::chrono::system_clock::time_point m_started_at;
  std::vector<File> m_files;

 public:
  StageRequest(std::vector<File> files);
  std::vector<File> const& files() const;
  std::vector<File>& files();
  std::chrono::system_clock::time_point const& created_at() const;
  std::chrono::system_clock::time_point const& started_at() const;
};
} // namespace storm

#endif