#include "configuration.hpp"
#include "extended_attributes.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fmt/core.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <numeric>
#include <optional>
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

static PhysicalPath load_storage_area_root(YAML::Node const& root,
                                           std::string_view sa_name)
{
  if (!root.IsDefined()) {
    throw std::runtime_error{
        fmt::format("storage area '{}' has no root", sa_name)};
  }

  if (root.IsNull()) {
    throw std::runtime_error{
        fmt::format("storage area '{}' has an empty root", sa_name)};
  }

  fs::path root_path{root.as<std::string>("")};

  return PhysicalPath{root_path.lexically_normal()};
}

static LogicalPaths load_storage_area_access_points(YAML::Node const& node,
                                                    std::string_view sa_name)
{
  if (!node.IsDefined()) {
    throw std::runtime_error{
        fmt::format("storage area '{}' has no access-point", sa_name)};
  }

  if (node.IsNull()) {
    throw std::runtime_error{
        fmt::format("storage area '{}' has an empty access-point", sa_name)};
  }

  if (LogicalPath access_point{node.as<std::string>("")};
      !access_point.empty()) {
    return {LogicalPath{access_point.lexically_normal()}};
  }

  using Strings = std::vector<std::string>;
  auto strings  = node.as<Strings>(Strings{});
  LogicalPaths paths;
  paths.reserve(strings.size());
  std::transform(strings.begin(), strings.end(), std::back_inserter(paths),
                 [](auto& s) { return LogicalPath{std::move(s)}; });
  return paths;
}

static StorageArea load_storage_area(YAML::Node const& sa)
{
  std::string name = load_storage_area_name(sa["name"]);
  PhysicalPath root{load_storage_area_root(sa["root"], name)};
  LogicalPaths ap{load_storage_area_access_points(sa["access-point"], name)};
  return {name, root, ap};
}

static void check_root(StorageArea const& sa, bool mirror_mode)
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

  if (!mirror_mode && !storm_has_all_permissions(root)) {
    throw std::runtime_error{
        fmt::format("root '{}' of storage area '{}' has invalid permissions",
                    root.string(), name)};
  }
}

static StorageAreas load_storage_areas(YAML::Node const& sas)
{
  StorageAreas result;

  for (auto& sa : sas) {
    result.emplace_back(load_storage_area(sa));
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
    using Aps = std::vector<std::pair<LogicalPath, StorageArea const*>>;
    auto aps  = std::transform_reduce(
        result.begin(), result.end(), Aps{},
        [](Aps r1, Aps r2) {
          r1.reserve(r1.size() + r2.size());
          std::move(r2.begin(), r2.end(), std::back_inserter(r1));
          return r1;
        },
        [](auto& sa) {
          Aps partial;
          auto const& sa_aps = sa.access_points;
          partial.reserve(sa_aps.size());
          std::transform(sa_aps.begin(), sa_aps.end(),
                          std::back_inserter(partial), [&](auto& ap) {
                           return std::pair{ap, &sa};
                         });
          return partial;
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
    // two storage areas cannot have an access point in common
    auto it = std::adjacent_find(
        access_points.begin(),
        access_points.end(), //
        [](auto const& l, auto const& r) { return l.first == r.first; });

    if (it != access_points.end()) {
      throw std::runtime_error{fmt::format(
          "storage areas '{}' and '{}' have the access point '{}' in common",
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

  return result;
}

static std::optional<std::uint16_t> load_port(YAML::Node const& node)
{
  if (!node.IsDefined()) {
    return {};
  }

  if (node.IsNull()) {
    throw std::runtime_error{fmt::format("port is null")};
  }

  int port;
  if (boost::conversion::try_lexical_convert(node, port)) {
    if (port > 0 && port < 65536) {
      return static_cast<std::uint16_t>(port);
    }
  }
  throw std::runtime_error{"invalid 'port' entry in configuration"};
}

static std::optional<LogLevel> load_log_level(YAML::Node const& node)
{
  if (!node.IsDefined()) {
    return {};
  }

  if (node.IsNull()) {
    throw std::runtime_error{fmt::format("log-level is null")};
  }

  int log_level;
  if (boost::conversion::try_lexical_convert(node, log_level)) {
    if (log_level >= 0 && log_level <= 4) {
      return log_level;
    }
  }
  throw std::runtime_error{"invalid 'log-level' entry in configuration"};
}

static std::optional<bool> load_mirror_mode(YAML::Node const& node)
{
  if (!node.IsDefined()) {
    return {};
  }

  if (node.IsNull()) {
    throw std::runtime_error{fmt::format("mirror-mode is null")};
  }

  bool value;
  if (YAML::convert<bool>::decode(node, value)) {
    return value;
  }
  throw std::runtime_error{"invalid 'mirror-mode' entry in configuration"};
}

static Configuration load(YAML::Node const& node)
{
  const auto sas_key = "storage-areas";
  const auto& sas    = node[sas_key];

  if (!sas) {
    throw std::runtime_error{
        fmt::format("no '{}' entry in configuration", sas_key)};
  }

  Configuration config;
  config.storage_areas = load_storage_areas(sas);

  auto const port_key   = "port";
  auto const& port_s    = node[port_key];
  auto const maybe_port = load_port(port_s);
  if (maybe_port.has_value()) {
    config.port = *maybe_port;
  }

  auto const log_level_key   = "log-level";
  auto const& log_level_s    = node[log_level_key];
  auto const maybe_log_level = load_log_level(log_level_s);
  if (maybe_log_level.has_value()) {
    config.log_level = *maybe_log_level;
  }

  {
    auto const key    = "mirror-mode";
    auto const& value = node[key];
    auto const maybe  = load_mirror_mode(value);
    if (maybe.has_value()) {
      config.mirror_mode = *maybe;
    }
  }

  return config;
}

auto check_sa_roots(StorageAreas const& sas, bool mirror_mode)
{
  std::for_each(sas.begin(), sas.end(),
                [=](auto& sa) { check_root(sa, mirror_mode); });
}

Configuration load_configuration(std::istream& is)
{
  YAML::Node const node = YAML::Load(is);
  auto config           = load(node);
  check_sa_roots(config.storage_areas, config.mirror_mode);
  return config;
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
