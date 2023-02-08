#include "stage_response.hpp"

namespace storm {

crow::response StageResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

StageId const& StageResponse::id() const
{
  return m_id;
}

} // namespace storm