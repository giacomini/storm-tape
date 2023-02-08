#ifndef RELEASE_RESPONSE_HPP
#define RELEASE_RESPONSE_HPP

#include "types.hpp"
#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {

class StageRequest;

class ReleaseResponse
{
 private:
  StageId m_id;
  StageRequest const* m_stage{nullptr};
  Paths m_invalid;

 public:
  ReleaseResponse() = default;
  ReleaseResponse(StageRequest const* stage)
      : m_stage(stage)
  {}
  ReleaseResponse(StageId id, Paths invalid)
      : m_id(std::move(id))
      , m_invalid(std::move(invalid))
  {}

  StageId const& id() const { return m_id; }
  StageRequest const* stage() const { return m_stage; }
  Paths const& invalid() const {return m_invalid; }
};

} // namespace storm

#endif