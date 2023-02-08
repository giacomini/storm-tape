#include "routes.hpp"
#include "archive_response.hpp"
#include "cancel_response.hpp"
#include "configuration.hpp"
#include "database.hpp"
#include "delete_response.hpp"
#include "io.hpp"
#include "release_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "tape_service.hpp"

namespace storm {

void create_routes(crow::SimpleApp& app, Configuration const& config,
                   TapeService& service)
{
  namespace fs   = std::filesystem;
  namespace json = boost::json;

  app.route_dynamic(config.well_known_uri.c_str())([&] {
    return crow::response{
        crow::status::OK, "json",
        json::serialize(json::value{
            {"sitename", "cnaf-tape"},
            {"description", "This is the INFN-CNAF tape REST API endpoint"},
            {"endpoints",
             {{{"uri", config.api_uri},
               {"version", "v1"},
               {"metadata", {}}}}}})};
  });

  CROW_ROUTE(app, "/api/v1/stage")
      .methods("POST"_method)([&](crow::request const& req) {
        try {
          auto files = from_json(req.body);
          StageRequest request{files};
          auto resp = service.stage(request);
          return to_crow_response(resp, get_host(req, config));
        } catch (...) {
          return StageResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
  ([&](StageId const& id) {
    try {
      auto resp = service.status(id);
      if (resp.stage() == std::nullopt) {
        return StatusResponse::not_found();
      }
      return to_crow_response(resp);
    } catch (...) {
      return StatusResponse::bad_request();
    }
  });

  CROW_ROUTE(app, "/api/v1/stage/<string>/cancel")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              CancelRequest const cancel{req.body};
              auto resp = service.cancel(StageId{id}, cancel);
              if (resp.id().empty()) {
                return CancelResponse::not_found();
              }
              if (resp.invalid().empty()) {
                return CancelResponse::cancelled();
              }
              return to_crow_response(resp);
            } catch (...) {
              return CancelResponse::bad_request();
            }
          });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
      .methods("DELETE"_method)([&](std::string const& id) {
        try {
          auto resp = service.erase(StageId{id});
          return resp.found() ? DeleteResponse::erased()
                              : DeleteResponse::not_found();
        } catch (...) {
          return DeleteResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/api/v1/release/<string>")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              ReleaseRequest const release{req.body};
              auto resp = service.release(StageId{id}, release);
              if (resp.stage() == nullptr) {
                return ReleaseResponse::not_found();
              }
              if (resp.invalid().empty()) {
                return ReleaseResponse::released();
              }
              return to_crow_response(resp);
            } catch (...) {
              return ReleaseResponse::bad_request();
            }
          });

  CROW_ROUTE(app, "/api/v1/archiveinfo")
      .methods("POST"_method)([&](crow::request const& req) {
        try {
          ArchiveInfo const info{req.body};
          auto resp = service.archive(info);
          return to_crow_response(resp);
        } catch (...) {
          return ArchiveResponse::bad_request();
        }
      });

  //  CROW_ROUTE(app, "/shutdown")
  //  ([&app]() {
  //    app.stop();
  //    return "Server shutdown";
  //  });

  CROW_ROUTE(app, "/gemss")([] { return crow::response{crow::status::OK}; });

  CROW_ROUTE(app, "/favicon.ico")
  ([] { return crow::response{crow::status::NO_CONTENT}; });
}

} // namespace storm