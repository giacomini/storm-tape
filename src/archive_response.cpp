#include "archive_response.hpp"

namespace storm {

boost::json::array const& ArchiveResponse::jbody() const
{
  return m_jbody;
}

crow::response
ArchiveResponse::fetched_from_archive(boost::json::array jbody) const
{
  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

} // namespace storm