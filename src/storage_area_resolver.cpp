#include "storage_area_resolver.hpp"
#include "errors.hpp"
#include <algorithm>

namespace storm {

namespace {

auto prefix_match_size(LogicalPath const& p1, LogicalPath const& p2)
{
  auto [it, _] = std::mismatch(p1.begin(), p1.end(), p2.begin(), p2.end());
  return std::distance(p1.begin(), it);
}

} // namespace

PhysicalPath StorageAreaResolver::operator()(LogicalPath const& path) const
{
  if (path.is_relative() || path != path.lexically_normal()) {
    return PhysicalPath{};
  }

  BOOST_ASSERT(!m_sas.empty());
  // find sa whose access_point has longest prefix match with logical_path
  auto sa_it =
      std::max_element(m_sas.begin(), m_sas.end(),
                       [&](StorageArea const& sa1, StorageArea const& sa2) {
                         return prefix_match_size(path, sa1.access_point)
                              < prefix_match_size(path, sa2.access_point);
                       });
  auto rel_path = path.lexically_relative(sa_it->access_point);
  BOOST_ASSERT(!rel_path.empty());

  if (rel_path == ".") {
    // logical_path exactly matches the access point
    return sa_it->root;
  }

  if (*rel_path.begin() == "..") {
    // no match
    return PhysicalPath{};
  }
  
  return static_cast<PhysicalPath>(sa_it->root / rel_path);
}

} // namespace storm
