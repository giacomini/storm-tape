#ifndef STAGE_RESPONSE_HPP
#define STAGE_RESPONSE_HPP

#include "types.hpp"
#include "file.hpp"
#include <boost/json.hpp>
#include <filesystem>
#include <string>

namespace storm {

class StageResponse
{
 private:
  StageId m_id{};
  Files m_files{};

 public:
  StageResponse() = default;
  explicit StageResponse(StageId id, Files&& files)
      : m_id(std::move(id)), m_files(files)
  {}

  StageId const& id() const { return m_id; }
  Files const& files() const { return m_files; }
  Files& files() { return m_files; }
};

} // namespace storm

#endif