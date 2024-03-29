#include "routes.hpp"
#include "archiveinfo_response.hpp"
#include "cancel_response.hpp"
#include "configuration.hpp"
#include "database.hpp"
#include "delete_response.hpp"
#include "errors.hpp"
#include "io.hpp"
#include "profiler.hpp"
#include "readytakeover_response.hpp"
#include "release_response.hpp"
#include "requests_with_paths.hpp"
#include "stage_request.hpp"
#include "stage_response.hpp"
#include "status_response.hpp"
#include "takeover_request.hpp"
#include "takeover_response.hpp"
#include "in_progress_response.hpp"
#include "tape_service.hpp"
#include <ctime>
#include <exception>

namespace storm {

void create_routes(CrowApp& app, Configuration const& config,
                   TapeService& service)
{
  CROW_ROUTE(app, "/api/v1/stage")
      .methods("POST"_method)([&](crow::request const& req) {
        PROFILE_SCOPE("STAGE");
        auto& access_logger = app.get_context<AccessLogger>(req);
        access_logger.operation = "STAGE";
        try {
          StageRequest request{from_json(req.body, StageRequest::tag),
                               std::time(nullptr), 0, 0};
          auto resp      = service.stage(std::move(request));
          auto crow_resp = to_crow_response(resp, get_hostinfo(req, config));
          access_logger.stage_id = resp.id();
          access_logger.files    = std::move(resp.files());
          return crow_resp;
        } catch (HttpError const& e) {
          CROW_LOG_ERROR << e.what() << '\n';
          return to_crow_response(e);
        } catch (std::exception const& e) {
          CROW_LOG_ERROR << e.what() << '\n';
          return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        } catch (...) {
          CROW_LOG_ERROR << "Unknown exception\n";
          return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }
      });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
  ([&](crow::request const& req, std::string const& id) {
    PROFILE_SCOPE("STATUS");
    app.get_context<AccessLogger>(req).operation = "STATUS";
    app.get_context<AccessLogger>(req).stage_id  = id;
    try {
      auto resp = service.status(StageId{id});
      return to_crow_response(resp);
    } catch (HttpError const& e) {
      CROW_LOG_ERROR << e.what() << '\n';
      return to_crow_response(e);
    } catch (std::exception const& e) {
      CROW_LOG_ERROR << e.what() << '\n';
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
    } catch (...) {
      CROW_LOG_ERROR << "Unknown exception\n";
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
    }
  });

  CROW_ROUTE(app, "/api/v1/stage/<string>/cancel")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            PROFILE_SCOPE("CANCEL");
            app.get_context<AccessLogger>(req).operation = "CANCEL";
            app.get_context<AccessLogger>(req).stage_id  = id;
            try {
              CancelRequest cancel{from_json(req.body, CancelRequest::tag)};
              auto resp = service.cancel(StageId{id}, std::move(cancel));
              if (resp.invalid.empty()) {
                return crow::response{crow::status::OK};
              }
              return to_crow_response(resp);
            } catch (HttpError const& e) {
              CROW_LOG_ERROR << e.what() << '\n';
              return to_crow_response(e);
            } catch (std::exception const& e) {
              CROW_LOG_ERROR << e.what() << '\n';
              return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            } catch (...) {
              CROW_LOG_ERROR << "Unknown exception\n";
              return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }
          });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
      .methods("DELETE"_method)(
          [&](crow::request const& req, std::string const& id) {
            PROFILE_SCOPE("DELETE");
            app.get_context<AccessLogger>(req).operation = "DELETE";
            app.get_context<AccessLogger>(req).stage_id  = id;
            try {
              auto const resp = service.erase(StageId{id});
              return to_crow_response(resp);
            } catch (HttpError const& e) {
              CROW_LOG_ERROR << e.what() << '\n';
              return to_crow_response(e);
            } catch (std::exception const& e) {
              CROW_LOG_ERROR << e.what() << '\n';
              return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            } catch (...) {
              CROW_LOG_ERROR << "Unknown exception\n";
              return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }
          });

  CROW_ROUTE(app, "/api/v1/release/<string>")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            PROFILE_SCOPE("RELEASE");
            app.get_context<AccessLogger>(req).operation = "RELEASE";
            app.get_context<AccessLogger>(req).stage_id  = id;
            try {
              ReleaseRequest release{from_json(req.body, ReleaseRequest::tag)};
              auto resp = service.release(StageId{id}, std::move(release));
              if (resp.invalid.empty()) {
                return crow::response{crow::status::OK};
              }
              return to_crow_response(resp);
            } catch (HttpError const& e) {
              CROW_LOG_ERROR << e.what() << '\n';
              return to_crow_response(e);
            } catch (std::exception const& e) {
              CROW_LOG_ERROR << e.what() << '\n';
              return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            } catch (...) {
              CROW_LOG_ERROR << "Unknown exception\n";
              return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }
          });

  CROW_ROUTE(app, "/api/v1/archiveinfo")
      .methods("POST"_method)([&](crow::request const& req) {
        PROFILE_SCOPE("ARCHIVEINFO");
        app.get_context<AccessLogger>(req).operation = "ARCHIVEINFO";
        try {
          ArchiveInfoRequest info{from_json(req.body, ArchiveInfoRequest::tag)};
          auto const resp = service.archive_info(std::move(info));
          return to_crow_response(resp);
        } catch (HttpError const& e) {
          CROW_LOG_ERROR << e.what() << '\n';
          return to_crow_response(e);
        } catch (std::exception const& e) {
          CROW_LOG_ERROR << e.what() << '\n';
          return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        } catch (...) {
          CROW_LOG_ERROR << "Unknown exception\n";
          return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }
      });

  CROW_ROUTE(app, "/favicon.ico")
  ([] { return crow::response{crow::status::NO_CONTENT}; });
}

void create_internal_routes(CrowApp& app, storm::Configuration const&,
                            storm::TapeService& service)
{
  CROW_ROUTE(app, "/recalltable/cardinality/tasks/readyTakeOver")
  ([&](crow::request const& req) {
    PROFILE_SCOPE("READY");
    app.get_context<AccessLogger>(req).operation = "READY";
    try {
      auto const resp = service.ready_take_over();
      return to_crow_response(resp);
    } catch (std::exception const& e) {
      CROW_LOG_ERROR << e.what() << '\n';
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
    } catch (...) {
      CROW_LOG_ERROR << "Unknown exception\n";
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
    }
  });

  CROW_ROUTE(app, "/recalltable/tasks")
      .methods("PUT"_method)([&](crow::request const& req) {
        PROFILE_SCOPE("TAKE_OVER");
        app.get_context<AccessLogger>(req).operation = "TAKE_OVER";
        try {
          TakeOverRequest const take_over{
              from_body_params(req.body, TakeOverRequest::tag)};
          auto const resp = service.take_over(take_over);
          return to_crow_response(resp);
        } catch (HttpError const& e) {
          CROW_LOG_ERROR << e.what() << '\n';
          return to_crow_response(e);
        } catch (std::exception const& e) {
          CROW_LOG_ERROR << e.what() << '\n';
          return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        } catch (...) {
          CROW_LOG_ERROR << "Unknown exception\n";
          return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }
      });

  CROW_ROUTE(app, "/recalltable/in_progress")
  ([&](crow::request const& req) {
    PROFILE_SCOPE("IN_PROGRESS");
    app.get_context<AccessLogger>(req).operation = "IN_PROGRESS";
    try {
      auto in_progress = from_query_params(req.url_params, InProgressRequest::tag);
      auto resp = service.in_progress(in_progress);
      return to_crow_response(resp);
    } catch (HttpError const& e) {
      CROW_LOG_ERROR << e.what() << '\n';
      return to_crow_response(e);
    } catch (std::exception const& e) {
      CROW_LOG_ERROR << e.what() << '\n';
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
    } catch (...) {
      CROW_LOG_ERROR << "Unknown exception\n";
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
    }
  });
}

} // namespace storm