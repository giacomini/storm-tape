#ifndef DELETE_RESPONSE_HPP
#define DELETE_RESPONSE_HPP

#include <crow.h>
#include <string>

namespace storm {
class Configuration;
class StageRequest;

class DeleteResponse
{
 private:
  StageRequest const* m_stage;

 public:
  DeleteResponse(StageRequest const* stage)
      : m_stage(stage)
  {}
  DeleteResponse()
  {}

  StageRequest const* stage() const;
  static crow::response bad_request();
  static crow::response not_found();
  static crow::response erased(); 
};

} // namespace storm

#endif