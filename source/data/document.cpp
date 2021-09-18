#include "document.h"

#include <fstream>

#include <json/json.hpp>

using json = nlohmann::json;

document from_file(const char* file_name) {
    std::ifstream i(file_name);
    json j;
    i >> j;

    document d;


    auto& actions = j.at("view_actions");
    d.view_actions.resize(actions.size());

    auto json_call = actions.begin();
    auto call = d.view_actions.begin();

    for (; json_call != actions.end(); ++json_call, ++call) {
        auto& shaders = json_call.value().at("shaders");

        call->vertex_shader = shaders.at("vertex").get<std::string>();
        call->fragment_shader = shaders.at("fragment").get<std::string>();
        call->viewport_width = 1280;
        call->viewport_height = 720;
        call->out.push_back({"outColor", {0, texture_usage::present}});
    }

    return d;
}
