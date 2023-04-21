#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "errors.hpp"

void boost::assertion_failed(char const* expr, char const* function,
                             char const* file, long line)
{
  std::cerr << "Failed assertion: '" << expr << "' in '" << function << "' ("
            << file << ':' << line << ")\n";
  REQUIRE(false);
}
