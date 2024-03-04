#include "io.hpp"
#include <crow/query_string.h>
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

TEST_CASE("An InProgressRequest accepts an `n > 0` and a `precise >= 0`, both "
          "optional")
{
  {
    auto r = storm::from_query_params(crow::query_string{"/?n=10&precise=1"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 10);
    CHECK_EQ(r.precise, 1);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 0);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?n=10"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 10);
    CHECK_EQ(r.precise, 0);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?precise=1"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 1);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?precise=2"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 2);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?n=0"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 0);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?n=-1"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 0);
  }
  {
    auto r = storm::from_query_params(crow::query_string{"/?precise=-1"},
                                      storm::InProgressRequest::tag);
    CHECK_EQ(r.n_files, 1'000);
    CHECK_EQ(r.precise, 0);
  }
}

TEST_SUITE_END;
