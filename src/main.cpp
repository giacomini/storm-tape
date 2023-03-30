#include "configuration.hpp"
#include "database.hpp"
#include "database_soci.hpp"
#include "local_storage.hpp"
#include "routes.hpp"
#include "tape_service.hpp"
#include <boost/program_options.hpp>
#include <fmt/core.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <filesystem>

namespace po = boost::program_options;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  try {
    std::string config_file;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
    ("help,h", "produce help message")
    ("config,c",
     po::value<std::string>(&config_file)->default_value("storm-tape.conf"),
     "specify configuration file"
    );
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return EXIT_SUCCESS;
    }

    auto const config = storm::load_configuration(fs::path{config_file});

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Debug);
    soci::session sql(soci::sqlite3, "storm-tape.sqlite");
    storm::SociDatabase db{sql};
    // storm::MockDatabase db{};
    storm::LocalStorage storage{};
    storm::TapeService service{config, db, storage};

    storm::create_routes(app, config, service);
    storm::create_internal_routes(app, config, service);

    // TODO add signals?
    app.port(config.port).concurrency(1).run();
  } catch (std::exception const& e) {
    CROW_LOG_CRITICAL << fmt::format("Caught exception: {}", e.what());
    return EXIT_FAILURE;
  } catch (...) {
    CROW_LOG_CRITICAL << "Caught unknown exception";
    return EXIT_FAILURE;
  }
}
