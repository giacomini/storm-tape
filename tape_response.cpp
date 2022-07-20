#include "tape_response.hpp"
#include "configuration.hpp"

crow::response TapeResponse::stage(json::object jbody,
                                   Configuration const& config)
{
  crow::response resp{crow::status::CREATED, "json", json::serialize(jbody)};
  std::string id = jbody.at("requestId").as_string().c_str();
  resp.add_header("Location", config.api_uri + "/stage/" + id);
  return resp;
}