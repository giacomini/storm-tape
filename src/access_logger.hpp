#ifndef STORM_ACCESS_LOGGER_HPP
#define STORM_ACCESS_LOGGER_HPP

#include "file.hpp"
#include <crow.h>
#include <string>

namespace storm {

struct AccessLogger
{
  struct context
  {
    std::string operation{"-"};
    std::string stage_id{"-"};
    Files files;
  };

  void before_handle(crow::request&, crow::response&, context&)
  {}

  void after_handle(crow::request& req, crow::response& res, context& ctx);
};

} // namespace storm

#endif
