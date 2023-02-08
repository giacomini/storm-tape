#include "status_response.hpp"

namespace storm {

crow::response
StatusResponse::status(boost::json::object const& jbody) const
{
  return crow::response{crow::status::OK, "json",
                        boost::json::serialize(jbody)};
}

StageId const& StatusResponse::id() const
{
  return m_id;
}

std::optional<StageRequest> const StatusResponse::stage() const
{
  return m_stage;
}

crow::response StatusResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

crow::response StatusResponse::not_found()
{
  return crow::response(crow::status::NOT_FOUND);
}

} // namespace storm