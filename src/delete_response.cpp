#include "delete_response.hpp"

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