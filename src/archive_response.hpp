#ifndef ARCHIVE_RESPONSE_HPP
#define ARCHIVE_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {
class Configuration;
class StageRequest;

class ArchiveResponse
{
 private:
  boost::json::array m_jbody;
  std::vector<std::filesystem::path> m_invalid;

 public:
  ArchiveResponse(boost::json::array& jbody)
      : m_jbody(std::move(jbody))
  {}
  ArchiveResponse(boost::json::array& jbody,
                  std::vector<std::filesystem::path> const& invalid)
      : m_jbody(std::move(jbody))
      , m_invalid(std::move(invalid))
  {}
  ArchiveResponse()
  {}

  boost::json::array const& jbody() const;
  std::vector<std::filesystem::path> const& invalid() const;

  crow::response fetched_from_archive(boost::json::array jbody) const;
  static crow::response bad_request();
};

} // namespace storm

#endif