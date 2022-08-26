#include "tape_response.hpp"
#include "configuration.hpp"

crow::response storm::TapeResponse::staged(boost::json::object jbody,
                                           storm::Configuration const& config)
{
  crow::response resp{crow::status::CREATED, "json",
                      boost::json::serialize(jbody)};
  resp.add_header("Location", config.api_uri + "/stage/" + m_id);
  return resp;
}

crow::response storm::TapeResponse::status(boost::json::object jbody)
{
  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

crow::response storm::TapeResponse::cancelled()
{
  return crow::response{crow::status::OK};
}

crow::response storm::TapeResponse::erased()
{
  return crow::response{crow::status::OK};
}

crow::response storm::TapeResponse::released()
{
  return crow::response{crow::status::OK};
}

crow::response
storm::TapeResponse::fetched_from_archive(boost::json::array jbody)
{
  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

crow::response
storm::TapeResponse::bad_request_with_body(boost::json::object jbody)
{
  return crow::response(crow::status::BAD_REQUEST, "json",
                        boost::json::serialize(jbody));
}

crow::response storm::TapeResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

crow::response storm::TapeResponse::not_found()
{
  return crow::response(crow::status::NOT_FOUND);
}

std::string storm::TapeResponse::getId() const
{
  return m_id;
}

std::vector<std::filesystem::path> storm::TapeResponse::getInvalid() const
{
  return m_invalid;
}
