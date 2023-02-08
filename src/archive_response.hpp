#ifndef ARCHIVE_RESPONSE_HPP
#define ARCHIVE_RESPONSE_HPP

#include "file.hpp"
#include "types.hpp"
#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {

class Configuration;
class StageRequest;

class ArchiveResponse
{
  boost::json::array m_jbody;
  Paths m_invalid;
  std::vector<File> m_valid;

 public:
  ArchiveResponse() = default;
  ArchiveResponse(boost::json::array jbody, std::vector<File> valid)
      : m_jbody(std::move(jbody))
      , m_valid(std::move(valid))
  {}
  ArchiveResponse(boost::json::array jbody,
                  Paths invalid,
                  std::vector<File> valid)
      : m_jbody(std::move(jbody))
      , m_invalid(std::move(invalid))
      , m_valid(std::move(valid))
  {}

  boost::json::array const& jbody() const;
  Paths const& invalid() const;
  std::vector<File> const& valid() const;

  crow::response fetched_from_archive(boost::json::array jbody) const;
  static crow::response bad_request();
};

} // namespace storm

#endif