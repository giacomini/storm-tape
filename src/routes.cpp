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

void create_routes(crow::SimpleApp& app, storm::Configuration const& config,
                   storm::TapeService& service)
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
          auto files = storm::from_json(req.body);
          storm::StageRequest request{files};
          auto resp = service.stage(request);
          return storm::to_crow_response(resp, storm::get_host(req));
        } catch (...) {
          return storm::StageResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
  ([&](std::string const& id) {
    try {
      auto resp = service.status(id);
      if (resp.stage() == nullptr) {
        return storm::StatusResponse::not_found();
      }
      return storm::to_crow_response(resp);
    } catch (...) {
      return storm::StatusResponse::bad_request();
    }
  });

  CROW_ROUTE(app, "/api/v1/stage/<string>/cancel")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              storm::CancelRequest cancel{req.body};
              auto resp = service.cancel(id, cancel);
              if (resp.stage() == nullptr) {
                return storm::CancelResponse::not_found();
              }
              if (resp.invalid().empty()) {
                return storm::CancelResponse::cancelled();
              }
              return storm::to_crow_response(resp);
            } catch (...) {
              return storm::CancelResponse::bad_request();
            }
          });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
      .methods("DELETE"_method)([&](std::string const& id) {
        try {
          auto resp = service.erase(id);
          if (resp.stage() == nullptr) {
            return storm::DeleteResponse::not_found();
          }
          return storm::DeleteResponse::erased();
        } catch (...) {
          return storm::DeleteResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/api/v1/release/<string>")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              storm::ReleaseRequest release{req.body};
              auto resp = service.release(id, release);
              if (resp.stage() == nullptr) {
                return storm::ReleaseResponse::not_found();
              }
              if (resp.invalid().empty()) {
                return storm::ReleaseResponse::released();
              }
              return storm::to_crow_response(resp);
            } catch (...) {
              return storm::ReleaseResponse::bad_request();
            }
          });

  CROW_ROUTE(app, "/api/v1/archiveinfo")
      .methods("POST"_method)([&](crow::request const& req) {
        try {
          storm::ArchiveInfo info{req.body};
          auto resp = service.archive(info);
          return storm::to_crow_response(resp);
        } catch (...) {
          return storm::ArchiveResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/gemss")([] { return crow::response{crow::status::OK}; });

  CROW_ROUTE(app, "/favicon.ico")
  ([] { return crow::response{crow::status::NO_CONTENT}; });
}