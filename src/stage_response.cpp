#include "stage_response.hpp"
#include "configuration.hpp"

crow::response
storm::StageResponse::staged(boost::json::object const& jbody,
                             storm::Configuration const& config) const
{
  crow::response resp{crow::status::CREATED, "json",
                      boost::json::serialize(jbody)};
  resp.add_header("Location", config.api_uri + "/stage/" + m_id);
  return resp;
}

crow::response storm::StageResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

std::string const& storm::StageResponse::id() const
{
  return m_id;
}