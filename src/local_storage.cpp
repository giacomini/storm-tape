#include "local_storage.hpp"
#include "extended_attributes.hpp"
#include <sys/stat.h>

namespace storm {

Locality LocalStorage::locality(Path const& p)
{
  struct stat sb = {};

  if (::stat(p.c_str(), &sb) == -1) {
    return Locality::unavailable;
  }

  if (sb.st_size == 0) {
    return Locality::none;
  }

  constexpr auto bytes_per_block{512L};
  bool const is_stub{sb.st_blocks * bytes_per_block < sb.st_size};
  bool const is_on_tape{[&] {
    XAttrName const storm_migrated{"user.storm.migrated"};
    std::error_code ec;
    return has_xattr(p, storm_migrated, ec);
  }()};

  if (is_stub) {
    return is_on_tape ? Locality::tape : Locality::lost;
  } else {
    return is_on_tape ? Locality::disk_and_tape : Locality::disk;
  }
}

} // namespace storm
