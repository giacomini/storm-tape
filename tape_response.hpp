#ifndef TAPE_RESPONSE_HPP
#define TAPE_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <string>

namespace json = boost::json;

class Configuration;

class TapeResponse
{
 public:
  static crow::response stage(json::object jbody, Configuration const& config);
};

#endif