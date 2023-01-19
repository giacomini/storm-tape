#include "stage_response.hpp"
#include "configuration.hpp"

crow::response
storm::StageResponse::staged(boost::json::object const& jbody,
                             std::map<std::string,std::string> const& map) const
{
  crow::response resp{crow::status::CREATED, "json",
                      boost::json::serialize(jbody)};
  resp.set_header("Location", map.at("proto") + "://" + map.at("host") + "/api/v1/stage/" + m_id);
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