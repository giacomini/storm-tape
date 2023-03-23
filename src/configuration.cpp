#include "configuration.hpp"
#include "extended_attributes.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fmt/core.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>

namespace algo = boost::algorithm;

namespace storm {

static std::string load_storage_area_name(YAML::Node const& name)
{
  if (!name.IsDefined()) {
    throw std::runtime_error{"there is a storage area with no name"};
  }

  if (name.IsNull()) {
    throw std::runtime_error{"there is a storage area with an empty name"};
  }

  auto result = name.as<std::string>();

  if (result.empty()) {
    throw std::runtime_error{
        "there is a storage area with an empty string name"};
  }

  static std::regex const re{"^[a-zA-Z][a-zA-Z0-9-_.]*$"};

  if (std::regex_match(result, re)) {
    return result;
  } else {
    throw std::runtime_error{
        fmt::format("invalid storage area name '{}'", result)};
  }
}

static bool storm_has_all_permissions(fs::path const& path)
{
  auto tmp = path / boost::uuids::to_string(boost::uuids::random_generator{}());
  std::shared_ptr<void> guard{nullptr, [&](auto) {
                                std::error_code ec;
                                fs::remove(tmp, ec);
                              }};
  {
    // can create a file
    std::ofstream f(tmp);
    if (!f) {
      return false;
    }
  }
  {
    // can read a file
    std::ifstream f(tmp);
    if (!f) {
      return false;
    }
  }

  {
    XAttrName const tsm_rect{"user.TSMRecT"};
    std::error_code ec;

    // can create an xattr
    create_xattr(tmp, tsm_rect, ec);
    if (ec != std::error_code{}) {
      return false;
    }

    // can check an xattr
    has_xattr(tmp, tsm_rect, ec);
    if (ec != std::error_code{}) {
      return false;
    }
  }

  return true;
}

static fs::path load_storage_area_root(YAML::Node const& root)
{
  fs::path root_path{root.as<std::string>("")};

  return root_path.lexically_normal();
}

static fs::path load_storage_area_access_point(YAML::Node const& node)
{
  fs::path access_point{node.as<std::string>("")};

  return access_point.lexically_normal();
}

static StorageArea load_storage_area(YAML::Node const& sa)
{
  std::string name = load_storage_area_name(sa["name"]);
  fs::path root    = load_storage_area_root(sa["root"]);
  fs::path ap      = load_storage_area_access_point(sa["access-point"]);
  return {name, root, ap};
}

static void check_root(StorageArea const& sa)
{
  auto const& [name, root, _] = sa;

  if (!root.is_absolute()) {
    throw std::runtime_error{
        fmt::format("root '{}' of storage area '{}' is not an absolute path",
                    root.string(), name)};
  }

  std::error_code ec;
  auto const stat = fs::status(root, ec);

  if (!fs::exists(stat)) {
    throw std::runtime_error{fmt::format(
        "root '{}' of storage area '{}' does not exist", root.string(), name)};
  }

  if (ec != std::error_code{}) {
    throw std::runtime_error{
        fmt::format("root '{}' of storage area '{}' is not available",
                    root.string(), name)};
  }
  if (!fs::is_directory(stat)) {
    throw std::runtime_error{
        fmt::format("root '{}' of storage area '{}' is not a directory",
                    root.string(), name)};
  }

  if (!storm_has_all_permissions(root)) {
    throw std::runtime_error{
        fmt::format("root '{}' of storage area '{}' has invalid permissions",
                    root.string(), name)};
  }
}

static StorageAreas load_storage_areas(YAML::Node const& sas)
{
  StorageAreas result;

  for (auto& sa : sas) {
    result.push_back(load_storage_area(sa));
  }

  if (result.empty()) {
    throw std::runtime_error{
        "configuration error - empty 'storage-areas' entry"};
  }

  // keep storage areas sorted by name
  std::sort(result.begin(),
            result.end(), //
            [](StorageArea const& l, StorageArea const& r) {
              return algo::ilexicographical_compare(l.name, r.name);
            });

  // copy all the access points in another vector, together with a pointer to
  // the corresponding storage area
  auto const access_points = [&] {
    std::vector<std::pair<Path, StorageArea const*>> aps;
    aps.reserve(aps.size());
    std::transform(result.begin(), result.end(), std::back_inserter(aps),
                   [](StorageArea const& sa) {
                     return std::pair{sa.access_point, &sa};
                   });
    std::sort(aps.begin(), aps.end(),
              [](auto const& a, auto const& b) { return a.first < b.first; });
    return aps;
  }();

  {
    // storage areas cannot have the same name
    auto it =
        std::adjacent_find(result.begin(),
                           result.end(), //
                           [](StorageArea const& l, StorageArea const& r) {
                             return algo::iequals(l.name, r.name);
                           });

    if (it != result.end()) {
      throw std::runtime_error{
          fmt::format("two storage areas have the same name '{}'", it->name)};
    }
  }

  {
    // two storage areas cannot have the same access point
    auto it = std::adjacent_find(
        access_points.begin(),
        access_points.end(), //
        [](auto const& l, auto const& r) { return l.first == r.first; });

    if (it != access_points.end()) {
      throw std::runtime_error{fmt::format(
          "storage areas '{}' and '{}' have the same access point '{}'",
          it->second->name, std::next(it)->second->name, it->first.string())};
    }
  }

  {
    // all access points must be absolute
    auto it = std::find_if(access_points.begin(), access_points.end(),
                           [](auto const& e) { return e.first.is_relative(); });

    if (it != access_points.end()) {
      throw std::runtime_error{fmt::format(
          "access point '{}' of storage area '{}' is not an absolute path",
          it->first.string(), it->second->name)};
    }
  }

  {
    std::for_each(result.begin(), result.end(),
                  [](auto const& sa) { check_root(sa); });
  }

  return result;
}

static Configuration load(YAML::Node const& node)
{
  static auto key = "storage-areas";
  auto& sas       = node[key];

  if (!sas) {
    throw std::runtime_error{
        fmt::format("no '{}' entry in configuration", key)};
  }

  Configuration config;
  config.storage_areas = load_storage_areas(sas);

  return config;
}

Configuration load_configuration(std::istream& is)
{
  Configuration result;

  YAML::Node const config = YAML::Load(is);

  return load(config);
}

Configuration load_configuration(fs::path const& p)
{
  std::ifstream is(p);
  if (!is) {
    throw std::runtime_error{
        fmt::format("cannot open configuration file '{}'", p.string())};
  } else {
    return load_configuration(is);
  }
}

} // namespace storm
