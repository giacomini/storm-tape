#ifndef DELETE_RESPONSE_HPP
#define DELETE_RESPONSE_HPP

#include <crow.h>

namespace storm {

class Configuration;

class DeleteResponse
{
  bool m_found{};

 public:
  DeleteResponse(bool found)
      : m_found(found)
  {}
  bool found() const
  {
    return m_found;
  }
};

} // namespace storm

#endif