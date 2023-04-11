#include "local_storage.hpp"
#include "extended_attributes.hpp"
#include <sys/stat.h>

namespace storm {

Locality LocalStorage::locality(Path const& physical_path)
{
  struct stat sb = {};

  if (::stat(physical_path.c_str(), &sb) == -1) {
    return Locality::unavailable;
  }

  if (sb.st_size == 0) {
    return Locality::none;
  }

  constexpr auto bytes_per_block{512L};
  bool const is_stub{sb.st_blocks * bytes_per_block < sb.st_size};

  std::error_code ec;

  bool const is_being_recalled{[&] {
    XAttrName const tsm_rect{"user.TSMRecT"};
    return has_xattr(physical_path, tsm_rect, ec);
  }()};

  if (ec != std::error_code{}) {
    return Locality::unavailable;
  }

  bool const is_on_disk{!(is_stub || is_being_recalled)};

  bool const is_on_tape{[&] {
    XAttrName const storm_migrated{"user.storm.migrated"};
    return has_xattr(physical_path, storm_migrated, ec);
  }()};

  if (ec != std::error_code{}) {
    return Locality::unavailable;
  }

  if (is_on_disk) {
    return is_on_tape ? Locality::disk_and_tape : Locality::disk;
  } else {
    return is_on_tape ? Locality::tape : Locality::lost;
  }
}

} // namespace storm
