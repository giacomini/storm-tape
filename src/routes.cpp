#include "routes.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "configuration.hpp"
#include "database.hpp"
#include "delete_response.hpp"
#include "io.hpp"
#include "readytakeover_response.hpp"
#include "release_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include "tape_service.hpp"

namespace storm {

void create_routes(crow::SimpleApp& app, Configuration const& config,
                   TapeService& service)
{
  CROW_ROUTE(app, "/api/v1/stage")
      .methods("POST"_method)([&](crow::request const& req) {
        try {
          StageRequest request{from_json(req.body, StageRequest::tag)};
          auto resp = service.stage(std::move(request));
          return to_crow_response(resp, get_host(req, config));
        } catch (...) {
          return crow::response(crow::status::BAD_REQUEST);
        }
      });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
  ([&](std::string const& id) {
    try {
      auto resp = service.status(StageId{id});
      if (resp.id().empty()) {
        return crow::response(crow::status::NOT_FOUND);
      }
      return to_crow_response(resp);
    } catch (...) {
      return crow::response(crow::status::BAD_REQUEST);
    }
  });

  CROW_ROUTE(app, "/api/v1/stage/<string>/cancel")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              CancelRequest cancel{from_json(req.body, CancelRequest::tag)};
              auto resp = service.cancel(StageId{id}, std::move(cancel));
              if (resp.id.empty()) {
                return crow::response(crow::status::NOT_FOUND);
              }
              if (resp.invalid.empty()) {
                return crow::response{crow::status::OK};
              }
              return to_crow_response(resp);
            } catch (...) {
              return crow::response(crow::status::BAD_REQUEST);
            }
          });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
      .methods("DELETE"_method)([&](std::string const& id) {
        try {
          auto resp = service.erase(StageId{id});
          return resp.found() ? crow::response{crow::status::OK}
                              : crow::response{crow::status::NOT_FOUND};
        } catch (...) {
          return crow::response(crow::status::BAD_REQUEST);
        }
      });

  CROW_ROUTE(app, "/api/v1/release/<string>")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              ReleaseRequest release{from_json(req.body, ReleaseRequest::tag)};
              auto resp = service.release(StageId{id}, std::move(release));
              if (resp.stage() == nullptr) {
                return crow::response(crow::status::NOT_FOUND);
              }
              if (resp.invalid().empty()) {
                return crow::response{crow::status::OK};
              }
              return to_crow_response(resp);
            } catch (...) {
              return crow::response(crow::status::BAD_REQUEST);
            }
          });

  CROW_ROUTE(app, "/api/v1/archiveinfo")
      .methods("POST"_method)([&](crow::request const& req) {
        try {
          ArchiveInfoRequest info{from_json(req.body, ArchiveInfoRequest::tag)};
          auto const resp = service.archive_info(std::move(info));
          return to_crow_response(resp);
        } catch (...) {
          return crow::response(crow::status::BAD_REQUEST);
        }
      });

  CROW_ROUTE(app, "/favicon.ico")
  ([] { return crow::response{crow::status::NO_CONTENT}; });
}

void create_internal_routes(crow::SimpleApp& app, storm::Configuration const&,
                            storm::TapeService& service)
{
  CROW_ROUTE(app, "/recalltable/cardinality/tasks/readyTakeOver")
  ([&] {
    auto const resp = service.ready_take_over();
    return to_crow_response(resp);
  });

  CROW_ROUTE(app, "/recalltable/tasks")
      .methods("PUT"_method)([&](crow::request const& req) {
        TakeOverRequest const take_over{
            from_body_params(req.body, TakeOverRequest::tag)};
        auto const resp = service.take_over(take_over);
        return to_crow_response(resp);
      });
}

} // namespace storm