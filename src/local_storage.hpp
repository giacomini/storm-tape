#ifndef STORM_LOCALSTORAGE_HPP
#define STORM_LOCALSTORAGE_HPP

#include "storage.hpp"

namespace storm {

struct LocalStorage : Storage
{
  Locality locality(Path const&) override;
};

} // namespace storm

#endif
