#include "delete_response.hpp"

storm::StageRequest const* storm::DeleteResponse::stage() const
{
  return m_stage;
}

crow::response storm::DeleteResponse::bad_request()
{
  return crow::response(crow::status::BAD_REQUEST);
}

crow::response storm::DeleteResponse::not_found()
{
  return crow::response(crow::status::NOT_FOUND);
}

crow::response storm::DeleteResponse::erased()
{
  return crow::response{crow::status::OK};
}