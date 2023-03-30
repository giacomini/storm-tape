#ifndef STORM_STORAGE_AREA_RESOLVER_HPP
#define STORM_STORAGE_AREA_RESOLVER_HPP

#include "configuration.hpp"

namespace storm {

class StorageAreaResolver
{
  StorageAreas const& m_sas;

 public:
  StorageAreaResolver(StorageAreas const& sas)
      : m_sas{sas}
  {}
  Path operator()(Path const& logical_path) const;
};

} // namespace storm

#endif
