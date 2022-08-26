#include "routes.hpp"
#include "configuration.hpp"
#include "database.hpp"
#include "io.hpp"
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "tape_response.hpp"
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
          auto files = storm::fromJSONPath(req.body);
          storm::StageRequest stage{files};
          auto resp  = service.stage(stage);
          auto jbody = storm::newStageToJSON(resp.getId());
          return resp.staged(jbody, config);
        } catch (...) {
          return storm::TapeResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
  ([&](std::string const& id) {
    try {
      storm::StageRequest const* stage = service.find(id);
      if (stage == nullptr) {
        return storm::TapeResponse::not_found();
      }
      storm::TapeResponse resp{id};
      // TD: Decide if we have to create a TapeResponse or simply pass the
      // id to IO operations!
      auto jbody = storm::StagedToJSON(stage, resp.getId());
      return resp.status(jbody);
    } catch (...) {
      return storm::TapeResponse::bad_request();
    }
  });

  CROW_ROUTE(app, "/api/v1/stage/<string>/cancel")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              storm::StageRequest* stage = service.findAndEdit(id);
              if (stage == nullptr) {
                return storm::TapeResponse::not_found();
              }
              storm::Cancel cancel{req.body};

              auto staged_to_cancel = service.stagedToCancel(cancel, stage);
              // TD: These operations should be carried by service or a new
              // "utils" object?
              if (cancel.paths.size() != staged_to_cancel.size()) {
                auto resp = service.checkInvalid(cancel, staged_to_cancel, id);
                auto jbody =
                    storm::fileMissingToJSON(resp.getInvalid(), resp.getId());
                // TD: Decide also here if better a TapeResponse
                // or simply pass to IO operations!
                return resp.bad_request_with_body(jbody);
              } else {
                auto resp = service.cancel(cancel, stage);
                return resp.cancelled();
              }
            } catch (...) {
              return storm::TapeResponse::bad_request();
            }
          });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
      .methods("DELETE"_method)([&](std::string const& id) {
        try {
          storm::StageRequest const* stage = service.find(id);
          if (stage == nullptr) {
            return storm::TapeResponse::not_found();
          }
          auto resp = service.erase(id);
          return resp.erased();
        } catch (...) {
          return storm::TapeResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/api/v1/release/<string>")
      .methods(
          "POST"_method)([&](crow::request const& req, std::string const& id) {
        try {
          storm::StageRequest* stage = service.findAndEdit(id);
          if (stage == nullptr) {
            return storm::TapeResponse::not_found();
          }
          storm::Release release{req.body};

          // FG what does "valid request" mean? like for cancel? just
          // syntactically correct?
          // TD I believe it's an existing request with a certain ID

          auto staged_to_release = service.stagedToRelease(release, stage);

          if (release.paths.size() != staged_to_release.size()) {
            auto resp  = service.checkInvalid(release, staged_to_release, id);
            auto jbody = storm::fileMissingToJSON(
                resp.getInvalid(),
                resp.getId()); // Decide also here if better a TapeResponse
                               // or simply pass to IO operations!
            return resp.bad_request_with_body(jbody);
          }
          // FG what does "release all provided files" mean at the
          //"API endpoint"?
          // TD Good question!
          else {
            auto resp =
                service.release(release, stage); // TD: What do I have to do??
            return resp.released();
          }
        } catch (...) {
          return storm::TapeResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/api/v1/archiveinfo")
      .methods("POST"_method)([&](crow::request const& req) {
        try {
          boost::json::array jbody;
          storm::Archiveinfo info{req.body};

          auto from_archive = service.archive();
          jbody.reserve(info.paths.size());

          auto info_from_archive = service.infoFromArchive(info, from_archive);

          if (info.paths.size() != info_from_archive.size()) {
            auto resp = service.checkInvalid(info, info_from_archive);
            jbody     = storm::fileNotInArchiveToJSON(resp.getInvalid(), jbody);
            auto remaining = service.compute_remaining(info, resp.getInvalid());
            jbody          = storm::infoFromFilesToJSON(remaining, jbody);
            return resp.fetched_from_archive(jbody);
          } else {
            storm::TapeResponse
                resp; // Decide if we have to create a TapeResponse or simply
                      // pass the id to IO operations!
            jbody = storm::infoFromFilesToJSON(info.paths, jbody);
            return resp.fetched_from_archive(jbody);
          }
        } catch (...) {
          return storm::TapeResponse::bad_request();
        }
      });

  CROW_ROUTE(app, "/gemss")([] { return crow::response{crow::status::OK}; });

  CROW_ROUTE(app, "/favicon.ico")
  ([] { return crow::response{crow::status::NO_CONTENT}; });
}