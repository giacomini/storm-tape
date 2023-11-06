#include "storage_area_resolver.hpp"
#include "errors.hpp"
#include <algorithm>
#include <memory>
#include <numeric>

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
  // find the sa with an access_point that has longest prefix match with
  // logical_path
  // auto sa_it =
  //     std::max_element(m_sas.begin(), m_sas.end(),
  //                      [&](StorageArea const& sa1, StorageArea const& sa2) {
  //                        return prefix_match_size(path,
  //                        sa1.access_points.front())
  //                             < prefix_match_size(path,
  //                             sa2.access_points.front());
  //                      });
  // auto rel_path = path.lexically_relative(sa_it->access_points.front());
  struct BestMatch
  {
    LogicalPath const* path{nullptr};
    StorageArea const* sa{nullptr};
  };
  LogicalPath dummy;
  auto best_match = std::transform_reduce(
      m_sas.begin(), m_sas.end(), BestMatch{},
      [](BestMatch const& bm1, BestMatch const& bm2) {
        return bm1.path == nullptr ? bm2
            : bm2.path == nullptr ? bm1
            : std::max(bm1, bm2, [](auto& a, auto& b) {
                return std::distance(a.path->begin(), a.path->end())
                       < std::distance(b.path->begin(), b.path->end());
                   });
      },
      [&](StorageArea const& sa) {
        auto ap_it =
            std::max_element(sa.access_points.begin(), sa.access_points.end(),
                             [&](auto& ap1, auto& ap2) {
                               return prefix_match_size(path, ap1)
                                    < prefix_match_size(path, ap2);
                             });
        return prefix_match_size(path, *ap_it) == 0
                 ? BestMatch{}
                 : BestMatch{std::to_address(ap_it), &sa};
      });
  BOOST_ASSERT(best_match.path != &dummy);
  auto const rel_path = path.lexically_relative(*best_match.path);
  BOOST_ASSERT(!rel_path.empty());

  if (rel_path == ".") {
    // logical_path exactly matches the access point
    return best_match.sa->root;
  }

  if (*rel_path.begin() == "..") {
    // no match
    return PhysicalPath{};
  }

  return static_cast<PhysicalPath>(best_match.sa->root / rel_path);
}

} // namespace storm
