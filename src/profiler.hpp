#ifndef PROFILER_HPP
#define PROFILER_HPP

#include <algorithm>
#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

#define PROFILING 1
#ifdef PROFILING
#  define PROFILE_SCOPE(name) storm::InstrumentationTimer timer##__LINE__(name)
#  define PROFILE_FUNCTION()  PROFILE_SCOPE(__FUNCTION__)
#else
#  define PROFILE_SCOPE(name)
#  define PROFILE_FUNCTION()
#endif

namespace storm {

struct ProfileResult
{
  std::string name;
  long long start;
  long long end;
  std::size_t thread_id;
};

class Instrumentor
{
  std::ofstream m_os{"results.json"};
  bool m_first_profile{true};
  std::mutex m_lock;

  Instrumentor()
  {
    write_header();
  }

  void write_header()
  {
    m_os << R"({"otherData": {},"traceEvents":[)" << '\n';
  }

  void write_footer()
  {
    m_os << "]}";
  }

 public:
  static Instrumentor& Instance()
  {
    static Instrumentor instance;
    return instance;
  }

  ~Instrumentor()
  {
    write_footer();
  }

  void write_profile(const ProfileResult& result)
  {
    std::lock_guard lock(m_lock);

    if (m_first_profile) {
      m_first_profile = false;
    } else {
      m_os << ',';
    }

    auto name = result.name;
    std::replace(name.begin(), name.end(), '"', '\'');

    m_os << "{";
    m_os << R"("cat":"function",)";
    m_os << R"("dur":)" << result.end - result.start << ",";
    m_os << R"("name":")" << name << R"(",)";
    m_os << R"("ph":"X",)";
    m_os << R"("pid":0,)";
    m_os << R"("tid":)" << result.thread_id << ",";
    m_os << R"("ts":)" << result.start;
    m_os << "}\n";
  }
};

class InstrumentationTimer
{
  using Clock     = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  std::string m_name;
  TimePoint m_start_tp = Clock::now();

  void stop()
  {
    auto const start = std::chrono::duration_cast<std::chrono::microseconds>(
                           m_start_tp.time_since_epoch())
                           .count();
    auto const end = std::chrono::duration_cast<std::chrono::microseconds>(
                         Clock::now().time_since_epoch())
                         .count();
    auto const thread_id =
        std::hash<std::thread::id>{}(std::this_thread::get_id());

    Instrumentor::Instance().write_profile({m_name, start, end, thread_id});
  }

 public:
  InstrumentationTimer(std::string name)
      : m_name{std::move(name)}
  {}

  ~InstrumentationTimer()
  {
    try {
      stop();
    } catch (...) {
    }
  }
};

} // namespace storm

#endif
