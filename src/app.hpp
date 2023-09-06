#ifndef STORM_APP_HPP
#define STORM_APP_HPP

#include "access_logger.hpp"
#include <crow.h>

namespace storm {

using CrowApp = crow::App<AccessLogger>;

}

#endif
