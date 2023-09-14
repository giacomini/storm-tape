#include "access_logger.hpp"
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
bool acceptable_request_id(std::string_view id)
{
  std::string_view acceptable = "ABCDEFGHIJKLMNOPQRSTUVWZ"
                                "abcdefghijklmnopqrstuvwz"
                                "0123456789";
  return id.size() > 0U && id.size() <= 16U
      && id.find_first_not_of(acceptable) == std::string::npos;
}
} // namespace

void storm::AccessLogger::after_handle(crow::request& req, crow::response& res,
                                       context& ctx)
{
  auto const timestamp = [] {
    return fmt::format("{:%FT%T%Ez}", fmt::localtime(std::time({})));
  }();
  auto const request_id = [&] {
    auto id = req.get_header_value("x-request-id");
    return acceptable_request_id(id) ? id : "-";
  }();
  auto const principal = [&] {
    if (auto sub = req.get_header_value("x-sub"); !sub.empty()) {
      return sub;
    } else if (auto voms_user = req.get_header_value("x-voms_user");
               !voms_user.empty()) {
      std::ostringstream os;
      os << std::quoted(voms_user);
      return os.str();
    } else {
      return std::string{"-"};
    }
  }();
  auto files = [&](long space_left) {
    space_left -= 6; // for the square brackets, a comma and the ellipsis

    std::ostringstream result;
    result << '[';

    bool first = true;
    for (auto& file : ctx.files) {
      std::ostringstream os;
      os << std::quoted(file.logical_path.string());
      std::string qs{os.str()};
      if (std::ssize(qs) + (first ? 0 : 1) > space_left) { // consider the comma
        qs = "...";
      }
      if (first) {
        result << qs;
        first = false;
      } else {
        result << ',' << qs;
      }
      space_left -= std::ssize(qs);
      if (qs == "...") {
        break;
      }
    }
    result << ']';
    return result.str();
  };

  std::ostringstream os;
  os << timestamp << ' ' << request_id << ' ' << principal << ' '
     << ctx.operation << ' ' << res.code;
  if (ctx.operation == "STAGE" || ctx.operation == "STATUS"
      || ctx.operation == "CANCEL" || ctx.operation == "RELEASE"
      || ctx.operation == "DELETE") {
    os << ' ' << ctx.stage_id;
  }
  if (ctx.operation == "STAGE") {
    constexpr long max_line_length = 2'048;
    os << ' ' << files(max_line_length - static_cast<long>(os.tellp()));
  }
  std::cout << os.str() << '\n';
}
