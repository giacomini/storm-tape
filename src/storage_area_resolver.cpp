#include "storage_area_resolver.hpp"
#include <algorithm>
#include <cassert>

namespace storm {

namespace {

auto prefix_match_size(Path const& p1, Path const& p2)
{
  auto [it, _] = std::mismatch(p1.begin(), p1.end(), p2.begin(), p2.end());
  return std::distance(p1.begin(), it);
}

} // namespace

Path StorageAreaResolver::operator()(Path const& logical_path) const
{
  if (logical_path != logical_path.lexically_normal()) {
    return Path{};
  }

  // find sa whose access_point has longest prefix match with logical_path
  auto sa_it =
      std::max_element(m_sas.begin(), m_sas.end(),
                       [&](StorageArea const& sa1, StorageArea const& sa2) {
                         return prefix_match_size(logical_path, sa1.access_point)
                              < prefix_match_size(logical_path, sa2.access_point);
                       });
  auto rel_path = logical_path.lexically_relative(sa_it->access_point);
  assert(!rel_path.empty());

  if (rel_path == ".") {
    // logical_path exactly matches the access point
    return sa_it->root;
  }

  if (*rel_path.begin() == "..") {
    // no match
    return Path{};
  }

  return sa_it->root / rel_path;
}

} // namespace storm
