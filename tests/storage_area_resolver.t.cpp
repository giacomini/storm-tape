#include "storage_area_resolver.hpp"
#include <doctest.h>

TEST_SUITE_BEGIN("StorageAreaResolver");

TEST_CASE("Storage area resolver")
{
  // clang-format off

  storm::StorageAreas const sas{
    {"sa1", "/storage/atlas"        , "/atlas"},
    {"sa2", "/storage/atlas/scratch", "/atlasscratch"},
    {"sa3", "/storage/cms"          , "/cms"},
    {"sa4", "/storage2/cms"         , "/cms/data"}
  };
  storm::StorageAreaResolver resolve{sas};

  CHECK(resolve("/atlas/file")            == "/storage/atlas/file");
  CHECK(resolve("/atlas/")                == "/storage/atlas");
  CHECK(resolve("/atlas")                 == "/storage/atlas");
  CHECK(resolve("/atlas/dir/file")        == "/storage/atlas/dir/file");
  CHECK(resolve("/atlasscratch/file")     == "/storage/atlas/scratch/file");
  CHECK(resolve("/atlasscratch/dir/file") == "/storage/atlas/scratch/dir/file");
  CHECK(resolve("/atlassss/file")         == "");
  CHECK(resolve("/cms/file")              == "/storage/cms/file");
  CHECK(resolve("/cms/data/file")         == "/storage2/cms/file");
  CHECK(resolve("/cms/../atlas/file")     == "");

  // clang-format on
}

TEST_SUITE_END;
