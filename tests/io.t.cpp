#include "io.hpp"
#include <doctest.h>

TEST_SUITE_BEGIN("IO");

auto operator==(storm::HostInfo const& a, storm::HostInfo const& b)
{
  return a.proto == b.proto && a.host == b.host && a.port == b.port;
}

TEST_CASE("Testing HostInfo")
{
  // clang-format off

  {
    storm::HostInfo result{"http", "localhost", "8080"};
    fill_hostinfo_from_forwarded(result, "proto=http;host=localhost;port=8080");
    CHECK(result == storm::HostInfo("http", "localhost", "8080"));
  }
  {
    storm::HostInfo result{"http", "localhost", "8080"};
    fill_hostinfo_from_forwarded(result, "host=localhost;proto=http;port=8080");
    CHECK(result == storm::HostInfo("http", "localhost", "8080"));
  }
  {
    storm::HostInfo result{"http", "localhost", "8080"};
    fill_hostinfo_from_forwarded(result, "proto=http;host=localhost;port=80808080");
    CHECK(result == storm::HostInfo("http", "localhost", "80"));
  }
  {
    storm::HostInfo result{"http", "localhost", "8080"};
    fill_hostinfo_from_forwarded(result, "proto=https;host=tape.cnaf.infn.it;port=8080");
    CHECK(result == storm::HostInfo("https", "tape.cnaf.infn.it", "8080"));
  }
  {
    storm::HostInfo result{"http", "localhost", "8080"};
    fill_hostinfo_from_forwarded(result, "proto=https;host=tape.cnaf.infn.it");
    CHECK(result == storm::HostInfo("https", "tape.cnaf.infn.it", "443"));
  }
  {
    storm::HostInfo result{"http", "localhost", "8080"};
    fill_hostinfo_from_forwarded(result, "proto=http;host=tape.cnaf.infn.it");
    CHECK(result == storm::HostInfo("http", "tape.cnaf.infn.it", "80"));
  }
  {
    storm::HostInfo result{"http", "localhost", "8080"};
    fill_hostinfo_from_forwarded(result, "host=localhost;port=8081");
    CHECK(result == storm::HostInfo("http", "localhost", "8081"));
  }
  {
    storm::HostInfo result{"http", "localhost", "8080"};
    fill_hostinfo_from_forwarded(result, "");
    CHECK(result == storm::HostInfo("http", "localhost", "8080"));
  }
  // clang-format on
}

TEST_SUITE_END;
