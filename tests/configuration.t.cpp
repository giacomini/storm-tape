#include "configuration.hpp"
#include <doctest.h>
#include <fmt/core.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

TEST_SUITE_BEGIN("Configuration");

TEST_CASE("The storage-areas entry must exist")
{
  std::string const conf = R"()";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "no 'storage-areas' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The storage-areas entry cannot be empty")
{
  std::string const conf = R"(
storage-areas:
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "configuration error - empty 'storage-areas' entry",
                       std::runtime_error);
}

TEST_CASE("A storage area must have a name")
{
  std::string const conf = R"(
storage-areas:
- root: /tmp
  access-point: /someexp
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(there is a storage area with no name)",
                       std::runtime_error);
}

TEST_CASE("The name field of a storage area cannot be empty")
{
  std::string const conf = R"(
storage-areas:
- name: 
  root: /tmp
  access-point: /someexp
)";
  std::istringstream is(conf);

  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(there is a storage area with an empty name)",
                       std::runtime_error);
}

TEST_CASE("The name of a storage area cannot be the empty string")
{
  std::string const conf = R"(
storage-areas:
- name: ""
  root: /tmp
  access-point: /someexp
)";
  std::istringstream is(conf);

  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(there is a storage area with an empty string name)",
                       std::runtime_error);
}

TEST_CASE("The name of a storage area must be valid")
{
  std::string const conf = R"(
storage-areas:
- name: 7up
  root: /tmp
  access-point: /someexp
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(invalid storage area name '7up')",
                       std::runtime_error);
}

TEST_CASE("Two storage areas cannot have the same name")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /tmp1
  access-point: /someexp
- name: test
  root: /tmp2
  access-point: /someexp2
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(two storage areas have the same name 'test')",
                       std::runtime_error);
}

TEST_CASE("A storage area must have an access-point")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /tmp
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(storage area 'test' has no access-point)",
                       std::runtime_error);
}

TEST_CASE("A storage area must have at least one access-point")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point:
)";

  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(storage area 'test' has an empty access-point)",
                       std::runtime_error);
}

TEST_CASE("A single access point for a storage area can be expressed in "
          "multiple ways")
{
  std::string const conf = R"(
storage-areas:
- name: test1
  root: /tmp
  access-point: /data1
- name: test2
  root: /tmp
  access-point:
    - /data2
- name: test3
  root: /tmp
  access-point: [ "/data3" ]
)";

  std::istringstream is(conf);
  auto const configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 3);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  auto const& sa3 = configuration.storage_areas[2];
  CHECK_EQ(sa1.access_points, storm::LogicalPaths{"/data1"});
  CHECK_EQ(sa2.access_points, storm::LogicalPaths{"/data2"});
  CHECK_EQ(sa3.access_points, storm::LogicalPaths{"/data3"});
}

TEST_CASE("The access point of a storage area can be the filesystem root")
{
  std::string const conf = R"(
storage-areas:
- name: atlas
  root: /tmp
  access-point: /
)";
  std::istringstream is(conf);
  auto configuration = storm::load_configuration(is);
  CHECK_EQ(configuration.storage_areas[0].access_points,
           storm::LogicalPaths{"/"});
}

TEST_CASE("A storage area can have many access points")
{
  std::string const conf = R"(
storage-areas:
- name: atlas
  root: /tmp
  access-point:
    - /atlas1
    - /atlas2
- name: cms
  root: /tmp
  access-point: ["/cms1", "/cms2", "/cms3"]
)";
  std::istringstream is(conf);
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& atlas = configuration.storage_areas[0];
  auto const& cms   = configuration.storage_areas[1];
  CHECK_EQ(atlas.access_points, storm::LogicalPaths{"/atlas1", "/atlas2"});
  CHECK_EQ(cms.access_points, storm::LogicalPaths{"/cms1", "/cms2", "/cms3"});
}

TEST_CASE("The access points of a storage area are locally unique")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point:
    - /atlas1
    - /atlas2
    - /atlas1
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(storage areas 'test' and 'test' have the access point '/atlas1' in common)",
      std::runtime_error);
}

TEST_CASE("Two storage areas cannot have the same access point")
{
  std::string const conf = R"(
storage-areas:
- name: test1
  root: /tmp1
  access-point: /someexp
- name: test2
  root: /tmp2
  access-point: /someexp
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(storage areas 'test1' and 'test2' have the access point '/someexp' in common)",
      std::runtime_error);
}

TEST_CASE("Two storage areas cannot have an access point in common")
{
  std::string const conf = R"(
storage-areas:
- name: test1
  root: /tmp
  access-point:
    - /ap1
    - /ap2
    - /ap3
- name: test2
  root: /tmp
  access-point:
    - /ap4
    - /ap1
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(storage areas 'test1' and 'test2' have the access point '/ap1' in common)",
      std::runtime_error);
}

TEST_CASE("Two storage areas can have nested access points")
{
  fs::create_directory("/tmp/data1");
  fs::create_directory("/tmp/data2");
  std::string const conf = R"(
storage-areas:
- name: test1
  root: /tmp/data1
  access-point: /someexp
- name: test2
  root: /tmp/data2
  access-point: /someexp/somedir
)";
  std::istringstream is(conf);
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(
      sa2.access_points.front().lexically_relative(sa1.access_points.front()),
      "somedir");

  fs::remove_all("/tmp/data1");
  fs::remove_all("/tmp/data2");
}

TEST_CASE("A storage area can have nested access points")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point:
    - /someexp
    - /someexp/data
)";
  std::istringstream is(conf);
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 1);
  auto const& sa = configuration.storage_areas[0];
  REQUIRE_EQ(sa.access_points.size(), 2);
  CHECK_EQ(sa.access_points[1].lexically_relative(sa.access_points[0]), "data");
}

TEST_CASE("A storage area must have a root")
{
  std::string const conf = R"(
storage-areas:
- name: test
  access-point: /data
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(storage area 'test' has no root)",
                       std::runtime_error);
}

TEST_CASE("A storage area must have a non-empty root")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root:
  access-point: /data
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       R"(storage area 'test' has an empty root)",
                       std::runtime_error);
}

TEST_CASE("Two storage areas can have the same root")
{
  std::string const conf = R"(
storage-areas:
- name: test1
  root: /tmp
  access-point: /someexp
- name: test2
  root: /tmp
  access-point: /otherexp
)";
  std::istringstream is(conf);
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(sa1.root, sa2.root);
}

TEST_CASE("Two storage areas can have nested roots")
{
  fs::create_directory("/tmp/data");
  std::string const conf = R"(
storage-areas:
- name: test1
  root: /tmp
  access-point: /someexp
- name: test2
  root: /tmp/data
  access-point: /otherexp
)";
  std::istringstream is(conf);
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(sa2.root.lexically_relative(sa1.root), "data");

  fs::remove_all("/tmp/data");
}

TEST_CASE("Two storage areas can have nested access points and nested roots")
{
  fs::create_directory("/tmp/data");
  std::string const conf = R"(
storage-areas:
- name: test1
  root: /tmp
  access-point: /someexp
- name: test2
  root: /tmp/data
  access-point: /someexp/somedir
)";
  std::istringstream is(conf);
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(
      sa2.access_points.front().lexically_relative(sa1.access_points.front()),
      "somedir");
  CHECK_EQ(sa2.root.lexically_relative(sa1.root), "data");

  fs::remove_all("/tmp/data");
}

TEST_CASE("Two storage areas can have nested access points and nested roots, "
          "inverted")
{
  fs::create_directory("/tmp/data");
  std::string const conf = R"(
storage-areas:
- name: test1
  root: /tmp/data
  access-point: /someexp
- name: test2
  root: /tmp
  access-point: /someexp/somedir
)";
  std::istringstream is(conf);
  auto configuration = storm::load_configuration(is);
  REQUIRE_EQ(configuration.storage_areas.size(), 2);
  auto const& sa1 = configuration.storage_areas[0];
  auto const& sa2 = configuration.storage_areas[1];
  CHECK_EQ(
      sa2.access_points.front().lexically_relative(sa1.access_points.front()),
      "somedir");
  CHECK_EQ(sa1.root.lexically_relative(sa2.root), "data");

  fs::remove_all("/tmp/data");
}

TEST_CASE("The root of a storage area must be an absolute path")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: tmp
  access-point: /someexp
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(root 'tmp' of storage area 'test' is not an absolute path)",
      std::runtime_error);
}

TEST_CASE("The access point of a storage area must be an absolute path")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: someexp
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(access point 'someexp' of storage area 'test' is not an absolute path)",
      std::runtime_error);
}

TEST_CASE("The root of a storage area must be a directory")
{
  std::ofstream ofs("/tmp/file");
  std::string const conf = R"(
storage-areas:
- name: test
  root: /tmp/file
  access-point: /someexp
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(root '/tmp/file' of storage area 'test' is not a directory)",
      std::runtime_error);
  fs::remove(fs::path("/tmp/file"));
}

TEST_CASE("The root of a storage area must have the right permissions")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /usr/local
  access-point: /someexp
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(root '/usr/local' of storage area 'test' has invalid permissions)",
      std::runtime_error);
}

TEST_CASE("If the log level is not specified, it defaults to 1")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
)";
  std::istringstream is{conf};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.log_level, 1);
}

TEST_CASE("If present, the log level cannot be empty")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
log-level:
)";

  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "log-level is null",
                       std::runtime_error);
}

TEST_CASE("The log level is an integer between 0 and 4 included")
{
  auto constexpr sa_conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
)";

  for (int i : {0, 1, 2, 3, 4}) {
    auto conf = sa_conf + fmt::format("log-level: {}\n", i);
    std::istringstream is{conf};
    auto config = storm::load_configuration(is);
    CHECK_EQ(config.log_level, i);
  }

  for (int i : {-1, 5}) {
    auto conf = sa_conf + fmt::format("log-level: {}\n", i);
    std::istringstream is{conf};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "invalid 'log-level' entry in configuration",
                         std::runtime_error);
  }

  {
    auto conf = sa_conf + fmt::format("log-level: {}\n", 3.14);
    std::istringstream is{conf};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "invalid 'log-level' entry in configuration",
                         std::runtime_error);
  }

  {
    auto conf = sa_conf + fmt::format("log-level: {}\n", "foo");
    std::istringstream is{conf};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "invalid 'log-level' entry in configuration",
                         std::runtime_error);
  }
}

TEST_CASE("The configuration can be loaded from a stream")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
)";
  std::istringstream is(conf);
  auto configuration  = storm::load_configuration(is);
  auto& sa            = configuration.storage_areas.front();
  auto& name          = sa.name;
  auto& root          = sa.root;
  auto& access_points = sa.access_points;
  CHECK_EQ("test", name);
  CHECK_EQ(storm::PhysicalPath{"/tmp"}, root);
  CHECK_EQ(storm::LogicalPaths{"/someexp"}, access_points);
}

TEST_CASE("The configuration can be loaded from a file")
{
  std::ofstream ofs("/tmp/application.yml");
  std::string const conf = R"(storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
)";
  ofs << conf;
  ofs.close();
  auto configuration =
      storm::load_configuration(fs::path{"/tmp/application.yml"});
  auto& sa            = configuration.storage_areas.front();
  auto& name          = sa.name;
  auto& root          = sa.root;
  auto& access_points = sa.access_points;
  CHECK_EQ("test", name);
  CHECK_EQ(storm::PhysicalPath{"/tmp"}, root);
  CHECK_EQ(storm::LogicalPaths{"/someexp"}, access_points);

  fs::remove("/tmp/application.yml");
}

TEST_CASE("The configuration file must have the right permissions")
{
  fs::create_directory("/tmp/conf");
  std::ofstream ofs("/tmp/conf/application.yml");
  std::string const conf = R"(storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
)";
  ofs << conf;
  ofs.close();
  fs::permissions("/tmp/conf/application.yml", fs::perms::none);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(fs::path{"/tmp/conf/application.yml"}),
      R"(cannot open configuration file '/tmp/conf/application.yml')",
      std::runtime_error);

  fs::remove_all("/tmp/conf");
}

TEST_CASE("Mirror mode can have a boolean value")
{
  auto constexpr sa_conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
)";

  for (auto s : {"true", "on", "yes"}) {
    auto conf = sa_conf + fmt::format("mirror-mode: {}\n", s);
    std::istringstream is{conf};
    auto config = storm::load_configuration(is);
    CHECK_EQ(config.mirror_mode, true);
  }
  
  for (auto s : {"false", "off", "no"}) {
    auto conf = sa_conf + fmt::format("mirror-mode: {}\n", s);
    std::istringstream is{conf};
    auto config = storm::load_configuration(is);
    CHECK_EQ(config.mirror_mode, false);
  }
}

TEST_CASE("Mirror mode cannot have an integer value")
{
  auto constexpr sa_conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
)";

  for (int i : {0, 1}) {
    auto conf = sa_conf + fmt::format("mirror-mode: {}\n", i);
    std::istringstream is{conf};
    CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                         "invalid 'mirror-mode' entry in configuration",
                         std::runtime_error);
  }
}

TEST_CASE("If mirror mode is explicitly disabled, there is a write check on the SA root")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /usr/local
  access-point: /someexp
mirror-mode: false
)";
  std::istringstream is(conf);
  CHECK_THROWS_WITH_AS(
      storm::load_configuration(is),
      R"(root '/usr/local' of storage area 'test' has invalid permissions)",
      std::runtime_error);
}

TEST_CASE("If mirror mode is enabled, there is no write check on the SA root")
{
  std::string const conf = R"(
storage-areas:
- name: test
  root: /usr/local
  access-point: /someexp
mirror-mode: on
)";
  std::istringstream is(conf);
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.mirror_mode, true);
}

TEST_CASE("The default port must be 8080, if not specified")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
)";
  std::istringstream is{conf};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.port, 8080);
}

TEST_CASE("The port entry cannot be empty")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port:
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is), "port is null",
                       std::runtime_error);
}

TEST_CASE("An integer port between 1 and 65535 must succeed")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port: 1234
)";
  std::istringstream is{conf};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.port, 1234);
}

TEST_CASE("The port cannot be equal to '0'")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port: 0
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port cannot be negative")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port: -1
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port cannot be larger than 65535")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port: 65538
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port cannot be a floating point number")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port: 3.14
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port can be a string, if the numeric range is valid")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port: "8080"
)";
  std::istringstream is{conf};
  auto config = storm::load_configuration(is);
  CHECK_EQ(config.port, 8080);
}

TEST_CASE("The port cannot be a string representing an not valid integer")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port: foo
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_CASE("The port cannot be a string representing a floating point number")
{
  auto constexpr conf = R"(
storage-areas:
- name: test
  root: /tmp
  access-point: /someexp
port: "3.14"
)";
  std::istringstream is{conf};
  CHECK_THROWS_WITH_AS(storm::load_configuration(is),
                       "invalid 'port' entry in configuration",
                       std::runtime_error);
}

TEST_SUITE_END;
