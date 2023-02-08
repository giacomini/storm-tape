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
  explicit StageRequest(std::vector<File> files = {})
      : m_created_at(std::chrono::system_clock::now())
      , m_started_at(m_created_at)
      , m_files(std::move(files))
  {}
  std::vector<File> const& files() const
  {
    return m_files;
  }
  std::vector<File>& files()
  {
    return m_files;
  }
  auto created_at() const
  {
    return m_created_at;
  }
  auto started_at() const
  {
    return m_started_at;
  }
};

} // namespace storm

#endif