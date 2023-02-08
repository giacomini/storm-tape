#ifndef CANCEL_RESPONSE_HPP
#define CANCEL_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <optional>
#include <string>

#include "stage_request.hpp"

namespace storm {
class Configuration;

class CancelResponse
{
  StageId m_id;
  Paths m_invalid;

 public:
  CancelResponse(StageId id = {}, Paths invalid = {})
      : m_id(std::move(id))
      , m_invalid(std::move(invalid))
  {}

  StageId const& id() const { return m_id; }
  Paths const& invalid() const { return m_invalid; }
};

} // namespace storm

#endif