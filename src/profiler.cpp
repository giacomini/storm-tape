#include "profiler.hpp"
#include <fmt/core.h>

namespace storm {

void Instrumentor::write_profile(const ProfileResult& result)
{
  std::lock_guard lock(m_lock);

  if (m_first_profile) {
    m_first_profile = false;
  } else {
    m_os << ',';
  }

  auto name = result.name;
  std::replace(name.begin(), name.end(), '"', '\'');
  m_os << fmt::format(
      R"({{"cat":"function","dur":{},"name":"{}","ph":"X","pid":0,"tid":{},"ts":{}}})",
      result.end - result.start, name, result.thread_id, result.start)
       << '\n';
}

} // namespace storm
