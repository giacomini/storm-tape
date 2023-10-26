#ifndef STORM_STORAGE_HPP
#define STORM_STORAGE_HPP

#include "types.hpp"

namespace storm {

struct Storage
{
  virtual ~Storage()                                   = default;
  virtual Locality locality(PhysicalPath const& path) = 0;
};

} // namespace storm

#endif
