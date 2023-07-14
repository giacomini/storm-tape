#ifndef ERRORS_HPP
#define ERRORS_HPP

#define BOOST_ENABLE_ASSERT_HANDLER
#include "types.hpp"
#include <boost/assert.hpp>
#include <fmt/core.h>
#include <stdexcept>

namespace boost {
void assertion_failed(char const* expr, char const* function, char const* file,
                      long line);
}

namespace storm {

class Exception : public std::runtime_error
{
 public:
  using std::runtime_error::runtime_error;
  virtual int http_code() const = 0;
  virtual std::string detail() const
  {
    return {};
  }
};

class BadRequest : public Exception
{
 public:
  using Exception::Exception;
  int http_code() const override
  {
    return 400;
  }
};

class StageNotFound : public Exception
{
  static auto constexpr s_title     = "Stage not found";
  static auto constexpr s_format    = "Stage with id {} not found";
  static auto constexpr s_http_code = 404;

  StageId m_id;

 public:
  explicit StageNotFound(StageId id)
      : Exception(s_title)
      , m_id(std::move(id))
  {}

  int http_code() const override { return s_http_code; }

  std::string detail() const override {
    return fmt::format(s_format, m_id);
  }
};
} // namespace storm

#endif
