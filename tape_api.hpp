#ifndef TAPE_API_HPP
#define TAPE_API_HPP

#include <crow.h>

class Configuration;
class Database;

void create_routes(crow::SimpleApp& app, Configuration const& config,
                   Database& db);

#endif
