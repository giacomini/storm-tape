#include "storage_area_resolver.hpp"
#include "errors.hpp"
#include <algorithm>
#include <memory>
#include <numeric>
#include <optional>

namespace storm {

namespace {

using PrefixMatchOpt = std::optional<std::ptrdiff_t>;

PrefixMatchOpt prefix_match(LogicalPath const& path, LogicalPath const& ap)
{
  auto [_, it] = std::mismatch(path.begin(), path.end(), ap.begin(), ap.end());
  if (it == ap.end()) {
    // full prefix match
    return std::distance(ap.begin(), it);
  } else {
    return std::nullopt;
  }
}

struct BestMatch
{
  std::ptrdiff_t match_size;
  LogicalPath const* path{nullptr};
  StorageArea const* sa{nullptr};
  friend auto operator<(BestMatch const& bm1, BestMatch const& bm2)
  {
    return bm1.match_size < bm2.match_size;
  }
};

using BestMatchOpt = std::optional<BestMatch>;

} // namespace

PhysicalPath StorageAreaResolver::operator()(LogicalPath const& path) const
{
  if (path.is_relative() || path != path.lexically_normal()) {
    return PhysicalPath{};
  }

  BOOST_ASSERT(!m_sas.empty());

  // find the access point that is the longest prefix of the path, with the
  // corresponding SA
  auto best_match = std::transform_reduce(
      m_sas.begin(), m_sas.end(), BestMatchOpt{},
      [](BestMatchOpt const& bm1, BestMatchOpt const& bm2) {
        return std::max(bm1, bm2);
      },
      [&](StorageArea const& sa) -> BestMatchOpt {
        // find the access point in this SA that is the longest prefix of the
        // path, if it exists
        auto ap_it = std::max_element(
            sa.access_points.begin(), sa.access_points.end(),
            [&](auto& ap1, auto& ap2) {
              return prefix_match(path, ap1) < prefix_match(path, ap2);
            });
        if (auto m = prefix_match(path, *ap_it); m.has_value()) {
          return BestMatch{m.value(), std::to_address(ap_it),
                           std::addressof(sa)};
        } else {
          return std::nullopt;
        }
      });

  if (!best_match) {
    return PhysicalPath{};
  }

  auto const rel_path = path.lexically_relative(*best_match->path);
  BOOST_ASSERT(!rel_path.empty());

  if (rel_path == ".") {
    // path is exactly the access point
    return best_match->sa->root;
  }

  return static_cast<PhysicalPath>(best_match->sa->root / rel_path);
}

} // namespace storm
