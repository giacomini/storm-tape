#ifndef TAPE_API_H
#define TAPE_API_H
#include <boost/iterator_adaptors.hpp>
#include <boost/json.hpp>
#include <crow.h>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <algorithm>
#include <string>
#include "configuration.hpp"

void create_routes(crow::SimpleApp& app, Configuration const& config,
                          Database& db);

#endif
