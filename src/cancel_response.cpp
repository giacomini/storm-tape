#include "cancel_response.hpp"

namespace storm {

StageId const& CancelResponse::id() const
{
  return m_id;
}

Paths const& CancelResponse::invalid() const
{
  return m_invalid;
}

crow::response
CancelResponse::bad_request_with_body(boost::json::object jbody)
{
  return crow::response(crow::status::BAD_REQUEST, "json",
                        boost::json::serialize(jbody));
}

crow::response CancelResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

crow::response CancelResponse::not_found()
{
  return crow::response(crow::status::NOT_FOUND);
}

crow::response CancelResponse::cancelled()
{
  return crow::response{crow::status::OK};
}

} // namespace storm