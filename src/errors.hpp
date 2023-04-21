#ifndef ERRORS_HPP
#define ERRORS_HPP

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>

namespace boost {
void assertion_failed(char const* expr, char const* function, char const* file,
                      long line);
}

#endif
