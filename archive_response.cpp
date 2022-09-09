#include "archive_response.hpp"

boost::json::array const& storm::ArchiveResponse::jbody() const
{
  return m_jbody;
}

std::vector<std::filesystem::path> const&
storm::ArchiveResponse::invalid() const
{
  return m_invalid;
}

crow::response
storm::ArchiveResponse::fetched_from_archive(boost::json::array jbody) const
{
  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

crow::response storm::ArchiveResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}