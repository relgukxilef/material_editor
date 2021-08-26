#include "document.h"

#include <fstream>

#include <json/json.hpp>

using json = nlohmann::json;

document::document() {

}

document from_file(const char* file_name) {
    std::ifstream i(file_name);
    json j;
    i >> j;

    document d;


    auto& calls = j.at("calls");
    d.calls.resize(calls.size());

    auto json_call = calls.begin();
    auto call = d.calls.begin();

    for (; json_call != calls.end(); ++json_call, ++call) {
        auto& shaders = json_call.value().at("shaders");
        call->shader_modules.resize(shaders.size());

        auto json_shader = shaders.begin();
        auto shader = call->shader_modules.begin();

        for (; json_shader != shaders.end(); ++json_shader, ++shader) {
            *shader = json_shader.value().get<std::string>();
        }
    }

    return d;
}
