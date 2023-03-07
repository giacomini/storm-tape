#include "local_storage.hpp"
#include "extended_attributes.hpp"
#include <sys/stat.h>

namespace storm {

XAttrName const storm_migrated{"user.storm.migrated"};

Locality LocalStorage::locality(Path const& p)
{
  struct stat sb = {};

  if (::stat(p.c_str(), &sb) == -1) {
    return Locality::unavailable;
  }

  constexpr auto bytes_per_block{512L};
  bool const is_stub{sb.st_blocks * bytes_per_block < sb.st_size};
  // TODO check the presence of user.storm.migrated
  bool const is_migrated{[&] {
    std::error_code ec;
    return has_xattr(p, storm_migrated, ec);
  }()};

  if (is_stub) {
    // TODO unavailable -> lost? log something?
    return is_migrated ? Locality::tape : Locality::unavailable;
  } else {
    return is_migrated ? Locality::disk_and_tape : Locality::disk;
  }
}

} // namespace storm
