#ifndef STAGE_RESPONSE_HPP
#define STAGE_RESPONSE_HPP

#include "types.hpp"
#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {

class StageResponse
{
 private:
  StageId m_id;

 public:
  StageResponse() = default;
  StageResponse(StageId id)
      : m_id(std::move(id))
  {}

  static crow::response bad_request();

  StageId const& id() const;
};

} // namespace storm

#endif