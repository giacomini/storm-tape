#ifndef TAPE_API_HPP
#define TAPE_API_HPP

#include <crow.h>

namespace storm {
class Configuration;
class Database;
class TapeService;
} // namespace storm

void create_routes(crow::SimpleApp& app, storm::Configuration const& config,
                   storm::TapeService& service);

#endif
