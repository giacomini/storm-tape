#ifndef STATUS_RESPONSE_HPP
#define STATUS_RESPONSE_HPP

#include "stage_request.hpp"

namespace storm {

class StatusResponse
{
 private:
  StageId m_id{};
  StageRequest m_stage{};

 public:
  StatusResponse() = default;
  StatusResponse(StageId id, StageRequest stage)
      : m_id(std::move(id))
      , m_stage(std::move(stage))
  {}

  StageId const& id() const { return m_id; }
  StageRequest const& stage() const { return m_stage; }
};

} // namespace storm

#endif