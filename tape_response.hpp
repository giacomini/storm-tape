#ifndef TAPE_RESPONSE_HPP
#define TAPE_RESPONSE_HPP

#include <boost/json.hpp>
#include <crow.h>
#include <filesystem>
#include <string>

namespace storm {
class Configuration;

class TapeResponse
{
 private:
  std::string m_id;
  std::vector<std::filesystem::path> m_invalid;

 public:
  TapeResponse(std::string const& id,
               std::vector<std::filesystem::path>& invalid)
      : m_id(id)
      , m_invalid(invalid)
  {}
  TapeResponse(std::string const& id)
      : m_id(id)
  {}
  TapeResponse()
  {}

  crow::response staged(boost::json::object jbody, Configuration const& config);
  crow::response status(boost::json::object jbody);
  crow::response cancelled();
  crow::response erased();
  crow::response released();
  crow::response fetched_from_archive(boost::json::array jbody);
  crow::response bad_request_with_body(boost::json::object jbody);
  static crow::response bad_request();
  static crow::response not_found();

  std::string getId() const;
  std::vector<std::filesystem::path> getInvalid() const;
};

} // namespace storm

#endif