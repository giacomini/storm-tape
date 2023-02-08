#ifndef ARCHIVE_RESPONSE_HPP
#define ARCHIVE_RESPONSE_HPP

#include "file.hpp"
#include "types.hpp"
#include <boost/json.hpp>
#include <crow.h>

namespace storm {

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
  Paths const& invalid() const { return m_invalid; }
  std::vector<File> const& valid() const {return m_valid; }

  crow::response fetched_from_archive(boost::json::array jbody) const;
};

} // namespace storm

#endif