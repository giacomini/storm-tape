#include "storage_area_resolver.hpp"
#include <doctest.h>

TEST_SUITE_BEGIN("StorageAreaResolver");

// clang-format off

TEST_CASE("An invalid path is resolved to an empty path")
{
  storm::StorageAreas const sas{
    {"sa1", "/storage/atlas"        , {"/atlas"}},
    {"sa2", "/storage/atlas/scratch", {"/atlasscratch"}},
    {"sa3", "/storage/cms"          , {"/cms"}},
    {"sa4", "/storage2/cms"         , {"/cms/data"}}
  };
  storm::StorageAreaResolver resolve{sas};

  CHECK(resolve("/atlassss/file")         == "");
  CHECK(resolve("/cms/../atlas/file")     == "");
  CHECK(resolve("cms/file")               == "");
}

TEST_CASE("Resolve storage areas with nested roots")
{
  storm::StorageAreas const sas{
    {"sa1", "/storage/atlas"        , {"/atlas"}},
    {"sa2", "/storage/atlas/scratch", {"/atlasscratch"}}
  };
  storm::StorageAreaResolver resolve{sas};

  CHECK(resolve("/atlas/file")            == "/storage/atlas/file");
  CHECK(resolve("/atlas/")                == "/storage/atlas");
  CHECK(resolve("/atlas")                 == "/storage/atlas");
  CHECK(resolve("/atlas/dir/file")        == "/storage/atlas/dir/file");
  CHECK(resolve("/atlasscratch/file")     == "/storage/atlas/scratch/file");
  CHECK(resolve("/atlasscratch/dir/file") == "/storage/atlas/scratch/dir/file");
}

TEST_CASE("Resolve storage areas with nested access points")
{
  storm::StorageAreas const sas{
    {"sa3", "/storage/cms" , {"/cms"}},
    {"sa4", "/storage2/cms", {"/cms/data"}}
  };
  storm::StorageAreaResolver resolve{sas};

  CHECK(resolve("/cms/file")      == "/storage/cms/file");
  CHECK(resolve("/cms/data/file") == "/storage2/cms/file");
}

TEST_CASE("Resolve storage areas with nested access points and nested roots")
{
  storm::StorageAreas const sas{
    {"sa3", "/storage/cms"        , {"/cms"}},
    {"sa4", "/storage/cms/scratch", {"/cms/data"}}
  };
  storm::StorageAreaResolver resolve{sas};

  CHECK(resolve("/cms/file")      == "/storage/cms/file");
  CHECK(resolve("/cms/data/file") == "/storage/cms/scratch/file");
}

TEST_CASE("Resolve storage areas with nested access points and nested roots, inverted")
{
  storm::StorageAreas const sas{
    {"sa3", "/storage/cms/scratch", {"/cms"}},
    {"sa4", "/storage/cms"        , {"/cms/data"}}
  };
  storm::StorageAreaResolver resolve{sas};

  CHECK(resolve("/cms/file")      == "/storage/cms/scratch/file");
  CHECK(resolve("/cms/data/file") == "/storage/cms/file");
}

TEST_CASE("Resolve a storage area with multiple access points")
{
  storm::StorageAreas const sas{
    {"cms", "/storage/cms", {"/cms", "/cmsdata"}}
  };
  storm::StorageAreaResolver resolve{sas};

  CHECK(resolve("/cms/file")      == "/storage/cms/file");
  CHECK(resolve("/cmsdata/file")  == "/storage/cms/file");
  CHECK(resolve("/cms/data/file") == "/storage/cms/data/file");
}

TEST_CASE("Resolve a storage area with multiple nested access points")
{
  {
    storm::StorageAreas const sas{
      {"cms",   "/storage/cms",   {"/cms", "/cms/data"}},
      {"atlas", "/storage/atlas", {"/atlas/data", "/atlas"}},
      {"alice", "/storage/alice", {"/alice", "/"}}
    };
    storm::StorageAreaResolver resolve{sas};

    CHECK(resolve("/cms/file")        == "/storage/cms/file");
    CHECK(resolve("/cmsdata/file")    == "/storage/alice/cmsdata/file");
    CHECK(resolve("/cms/data/file")   == "/storage/cms/file");
    CHECK(resolve("/atlas/file")      == "/storage/atlas/file");
    CHECK(resolve("/atlasdata/file")  == "/storage/alice/atlasdata/file");
    CHECK(resolve("/atlas/data/file") == "/storage/atlas/file");
    CHECK(resolve("/alice/file")      == "/storage/alice/file");
    CHECK(resolve("/alicedata/file")  == "/storage/alice/alicedata/file");
    CHECK(resolve("/alice/data/file") == "/storage/alice/data/file");
  }

  {
    storm::StorageAreas const sas{
      {"cms",   "/storage/cms",   {"/cms", "/cms/data"}},
      {"atlas", "/storage/atlas", {"/atlas/data", "/atlas"}},
      {"alice", "/storage/alice", {"/", "/alice"}}
    };
    storm::StorageAreaResolver resolve{sas};

    CHECK(resolve("/alice/file")      == "/storage/alice/file");
    CHECK(resolve("/alicedata/file")  == "/storage/alice/alicedata/file");
    CHECK(resolve("/alice/data/file") == "/storage/alice/data/file");
  }
}

TEST_SUITE_END;
