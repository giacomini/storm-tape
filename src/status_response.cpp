#include "status_response.hpp"

crow::response
storm::StatusResponse::status(boost::json::object const& jbody) const
{
  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

std::string const& storm::StatusResponse::id() const
{
  return m_id;
}

storm::StageRequest const* storm::StatusResponse::stage() const
{
  return m_stage;
}

crow::response storm::StatusResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

crow::response storm::StatusResponse::not_found()
{
  return crow::response(crow::status::NOT_FOUND);
}