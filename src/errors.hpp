#ifndef ERRORS_HPP
#define ERRORS_HPP

#define BOOST_ENABLE_ASSERT_HANDLER
#include "types.hpp"
#include <boost/assert.hpp>
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
};

class BadRequest : public Exception
{
 public:
  using Exception::Exception;
  int http_code() const override { return 400; }
};
} // namespace storm

#endif
