#ifndef STORM_TAKEOVER_REQUEST_HPP
#define STORM_TAKEOVER_REQUEST_HPP

#include <cstddef>

namespace storm {

struct TakeOverRequest
{
  inline static constexpr struct Tag {} tag{};
  static constexpr std::size_t invalid{};
  std::size_t n_files;
};

} // namespace storm
#endif
