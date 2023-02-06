#include "cancel_response.hpp"

std::string const& storm::CancelResponse::id() const
{
  return m_id;
}

std::optional<storm::StageRequest> const storm::CancelResponse::stage() const
{
  return m_stage;
}

std::vector<std::filesystem::path> const& storm::CancelResponse::invalid() const
{
  return m_invalid;
}

crow::response
storm::CancelResponse::bad_request_with_body(boost::json::object jbody)
{
  return crow::response(crow::status::BAD_REQUEST, "json",
                        boost::json::serialize(jbody));
}

crow::response storm::CancelResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

crow::response storm::CancelResponse::not_found()
{
  return crow::response(crow::status::NOT_FOUND);
}

crow::response storm::CancelResponse::cancelled()
{
  return crow::response{crow::status::OK};
}