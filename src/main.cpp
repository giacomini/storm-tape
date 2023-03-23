#include "configuration.hpp"
#include "database.hpp"
#include "database_soci.hpp"
#include "local_storage.hpp"
#include "routes.hpp"
#include "tape_service.hpp"
#include <boost/program_options.hpp>
#include <soci/sqlite3/soci-sqlite3.h>
#include <filesystem>

namespace po = boost::program_options;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "config", po::value<std::string>(), "specify configuration file");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return EXIT_SUCCESS;
    }

    auto const config_file{[&] {
      if (vm.count("compression")) {
        return fs::path{vm["compression"].as<std::string>()};
      } else {
        return fs::path{"storm-tape.conf"};
      }
    }()};
    auto const config = storm::load_configuration(config_file);

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Debug);
    soci::session sql(soci::sqlite3, "storm-tape.sqlite");
    storm::SociDatabase db{sql};
    // storm::MockDatabase db{};
    storm::LocalStorage storage{};
    storm::TapeService service{db, storage};

    storm::create_routes(app, config, service);
    storm::create_internal_routes(app, config, service);

    // TODO add signals?
    app.port(config.port).concurrency(1).run();
  } catch (std::exception const& e) {
    std::cerr << "Caught exception: " << e.what() << '\n';
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Caught unknown exception\n";
    return EXIT_FAILURE;
  }
}
