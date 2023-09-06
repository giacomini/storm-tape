#include "access_logger.hpp"
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

void storm::AccessLogger::after_handle(crow::request& req, crow::response& res,
                                       context& ctx)
{
  auto const timestamp = [] {
    return fmt::format("{:%FT%T%Ez}", fmt::localtime(std::time({})));
  }();
  auto const request_id = [&] {
    auto result = req.get_header_value("X-Request-Id");
    if (result.empty()) {
      result = "-";
    }
    return result;
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
  os << timestamp << ' ' << request_id << ' ' << ctx.user << ' '
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
