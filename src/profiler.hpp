#ifndef PROFILER_HPP
#define PROFILER_HPP

#include <algorithm>
#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

#ifdef PROFILING
#  define COMBINE_HELPER(X,Y) X##Y
#  define COMBINE(X,Y) COMBINE_HELPER(X,Y)
#  define PROFILE_SCOPE(name) storm::InstrumentationTimer COMBINE(timer,__LINE__)(name)
#  define PROFILE_FUNCTION()  PROFILE_SCOPE(__PRETTY_FUNCTION__)
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
    static Instrumentor instance; //-V1096 TODO check
    return instance;
  }

  ~Instrumentor()
  {
    write_footer();
  }

  void write_profile(const ProfileResult& result);
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
