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
        auto type = json_call.value().at("type").get<std::string>();

        if (type == "program") {
            auto action = std::make_unique<program_action>();

            auto& shaders = json_call.value().at("shaders");

            action->vertex_shader = shaders.at("vertex").get<std::string>();
            action->fragment_shader = shaders.at("fragment").get<std::string>();
            action->viewport_width = 1280;
            action->viewport_height = 720;
            action->vertex_count =
                json_call.value().at("vertex_count").get<int>();
            action->out.push_back({"outColor", 0});

            *call = std::move(action);

        } else if (type == "blit") {
            auto action = std::make_unique<blit_action>();



            *call = std::move(action);

        } else {
            throw std::runtime_error("unsupported type");
        }
    }

    return d;
}
