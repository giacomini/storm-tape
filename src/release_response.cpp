#include "release_response.hpp"

std::string const& storm::ReleaseResponse::id() const
{
  return m_id;
}

storm::StageRequest const* storm::ReleaseResponse::stage() const
{
  return m_stage;
}

std::vector<std::filesystem::path> const&
storm::ReleaseResponse::invalid() const
{
  return m_invalid;
}

crow::response
storm::ReleaseResponse::bad_request_with_body(boost::json::object jbody)
{
  return crow::response(crow::status::BAD_REQUEST, "json",
                        boost::json::serialize(jbody));
}

crow::response storm::ReleaseResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

crow::response storm::ReleaseResponse::not_found()
{
  return crow::response(crow::status::NOT_FOUND);
}

crow::response storm::ReleaseResponse::released()
{
  return crow::response{crow::status::OK};
}