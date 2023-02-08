#include "archive_response.hpp"

namespace storm {

boost::json::array const& ArchiveResponse::jbody() const
{
  return m_jbody;
}

Paths const& ArchiveResponse::invalid() const
{
  return m_invalid;
}

std::vector<File> const& ArchiveResponse::valid() const
{
  return m_valid;
}

crow::response
ArchiveResponse::fetched_from_archive(boost::json::array jbody) const
{
  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

crow::response ArchiveResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

} // namespace storm