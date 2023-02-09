#ifndef ARCHIVEINFO_RESPONSE_HPP
#define ARCHIVEINFO_RESPONSE_HPP

#include "file.hpp"
#include "types.hpp"
#include <boost/json.hpp>
#include <crow.h>

namespace storm {

class ArchiveInfoResponse
{
  boost::json::array m_jbody;
  Paths m_invalid;
  std::vector<File> m_valid;

 public:
  ArchiveInfoResponse() = default;
  ArchiveInfoResponse(boost::json::array jbody, std::vector<File> valid)
      : m_jbody(std::move(jbody))
      , m_valid(std::move(valid))
  {}
  ArchiveInfoResponse(boost::json::array jbody,
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