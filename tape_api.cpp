#include "database.hpp"
#include "tape_api.hpp"
#include "configuration.hpp"

namespace json = boost::json;

void create_routes(crow::SimpleApp& app, Configuration const& config, Database& db){
  app.route_dynamic(config.well_known_uri.c_str())
  ([&] {
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
          StageRequest stage{req.body};
          auto jbody          = TapeService::stage(stage, db);
          crow::response resp = TapeResponse::stage(jbody, config);
          return resp;
        } catch (...) {
          return crow::response{crow::status::BAD_REQUEST};
        }
      });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
  ([&](std::string const& id) {
    try {
      StageRequest const* stage = db.find(id);
      if (stage == nullptr) {
        return crow::response{crow::status::NOT_FOUND};
      }
      json::array files;
      files.reserve(stage->files.size());
      std::transform( //
          stage->files.begin(), stage->files.end(), std::back_inserter(files),
          [](File const& file) {
            json::object result{{"path", file.path.c_str()}};
            if (file.locality == File::Locality::on_disk) {
              result.emplace("onDisk", true);
            } else {
              result.emplace("state", to_string(file.state));
            }
            return result;
          });
      json::object jbody;
      jbody["id"]         = id;
      jbody["created_at"] = //
          std::chrono::duration_cast<std::chrono::seconds>(
              stage->created_at.time_since_epoch())
              .count();
      jbody["started_at"] = //
          std::chrono::duration_cast<std::chrono::seconds>(
              stage->started_at.time_since_epoch())
              .count();
      jbody["files"] = files;
      return crow::response{crow::status::OK, "json", json::serialize(jbody)};
    } catch (...) {
      return crow::response{crow::status::BAD_REQUEST};
    }
  });

  CROW_ROUTE(app, "/api/v1/stage/<string>/cancel")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              StageRequest const* stage = db.find(id);
              if (stage == nullptr) {
                return crow::response{crow::status::NOT_FOUND};
              }
              Cancel cancel{req.body};

              auto proj = [](File const& stage_file) -> fs::path const& {
                return stage_file.path;
              };

              std::vector<fs::path> both;
              both.reserve(cancel.paths.size());

              std::set_intersection(
                  cancel.paths.begin(), cancel.paths.end(),
                  boost::make_transform_iterator(stage->files.begin(), proj),
                  boost::make_transform_iterator(stage->files.end(), proj),
                  std::back_inserter(both));

              if (cancel.paths.size() != both.size()) {
                std::vector<fs::path> invalid;
                assert(cancel.paths.size() > both.size());
                invalid.reserve(cancel.paths.size() - both.size());
                std::set_difference(cancel.paths.begin(), cancel.paths.end(),
                                    both.begin(), both.end(),
                                    std::back_inserter(invalid));
                std::string l;
                for (auto&& p : invalid) {
                  l += '\'';
                  l += p;
                  l += "' ";
                }
                json::object jbody{
                    {"title", "File missing from stage request"},
                    {"detail",
                     "Files " + l + "do not belong to stage request " + id}};
                return crow::response(crow::status::BAD_REQUEST, "json",
                                      serialize(jbody));
              }
              // cancel(cancel.paths);
              return crow::response{crow::status::OK};
            } catch (...) {
              return crow::response(crow::status::BAD_REQUEST);
            }
          });

  CROW_ROUTE(app, "/api/v1/stage/<string>")
      .methods("DELETE"_method)([&](std::string const& id) {
        try {
          StageRequest const* stage = db.find(id);
          if (stage == nullptr) {
            return crow::response{crow::status::NOT_FOUND};
          }
          // cancel(stage->files);
          auto const c = db.erase(id);
          assert(c == 1);
          return crow::response{crow::status::OK};
        } catch (...) {
          return crow::response{crow::status::BAD_REQUEST};
        }
      });

  CROW_ROUTE(app, "/api/v1/release/<string>")
      .methods("POST"_method)(
          [&](crow::request const& req, std::string const& id) {
            try {
              StageRequest const* stage = db.find(id);
              if (stage == nullptr) {
                return crow::response{crow::status::NOT_FOUND};
              }
              Release release{req.body};

              // FG what does "valid request" mean? like for cancel? just
              // syntactically correct?
              auto proj = [](File const& stage_file) -> fs::path const& {
                return stage_file.path;
              };

              std::vector<fs::path> both;
              both.reserve(release.paths.size());

              std::set_intersection(
                  release.paths.begin(), release.paths.end(),
                  boost::make_transform_iterator(stage->files.begin(), proj),
                  boost::make_transform_iterator(stage->files.end(), proj),
                  std::back_inserter(both));

              if (release.paths.size() != both.size()) {
                std::vector<fs::path> invalid;
                assert(release.paths.size() > both.size());
                invalid.reserve(release.paths.size() - both.size());
                std::set_difference(release.paths.begin(), release.paths.end(),
                                    both.begin(), both.end(),
                                    std::back_inserter(invalid));
                std::string l;
                for (auto&& p : invalid) {
                  l += '\'';
                  l += p;
                  l += "' ";
                }
                json::object jbody{
                    {"title", "File missing from stage request"},
                    {"detail",
                     "Files " + l + "do not belong to stage request " + id}};
                return crow::response(crow::status::BAD_REQUEST, "json",
                                      serialize(jbody));
              }
              // FG what does "release all provided files" mean at the "API
              // endpoint"?
              return crow::response{crow::status::OK};
            } catch (...) {
              return crow::response(crow::status::BAD_REQUEST);
            }
          });

  CROW_ROUTE(app, "/api/v1/archiveinfo")
      .methods("POST"_method)([&](crow::request const& req) {
        try {
          Archiveinfo info{req.body};
          json::array jbody;
          for (auto&& path : info.paths) {
          }
          return crow::response{crow::status::OK, "json",
                                json::serialize(jbody)};
        } catch (...) {
          return crow::response{crow::status::BAD_REQUEST};
        }
      });

  CROW_ROUTE(app, "/gemss")([] { return crow::response{crow::status::OK}; });

  CROW_ROUTE(app, "/favicon.ico")
  ([] { return crow::response{crow::status::NO_CONTENT}; });
}