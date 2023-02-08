#include "release_response.hpp"

namespace storm {

StageId const& ReleaseResponse::id() const
{
  return m_id;
}

StageRequest const* ReleaseResponse::stage() const
{
  return m_stage;
}

Paths const& ReleaseResponse::invalid() const
{
  return m_invalid;
}

crow::response
ReleaseResponse::bad_request_with_body(boost::json::object jbody)
{
  return crow::response(crow::status::BAD_REQUEST, "json",
                        boost::json::serialize(jbody));
}

crow::response ReleaseResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

crow::response ReleaseResponse::not_found()
{
  return crow::response(crow::status::NOT_FOUND);
}

crow::response ReleaseResponse::released()
{
  return crow::response{crow::status::OK};
}

} // namespace storm