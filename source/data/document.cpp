#include "document.h"

#include <fstream>

#include <glm/gtc/type_ptr.hpp>
#include <json/json.hpp>

uniform_value from_json(nlohmann::json json) {
    if (json.is_array()) {
        if (json.size() > 4)
            throw std::runtime_error(
                "Vectors and matrices larger than 4 not supported"
            );
        if (json.at(0).is_array()) {
            throw std::runtime_error(
                "Matrices larger than 4 not supported"
            );
            glm::mat4 matrix(1.0);
            int row_index = 0;
            for (const auto& row : json) {
                int column_index = 0;
                for (const auto& cell : row) {
                    matrix[row_index][column_index] = cell.get<float>();
                    ++column_index;
                }
                ++row_index;
            }

            return matrix;

        } else {
            glm::vec4 vector(0.0);
            int row_index = 0;
            for (const auto& cell : json) {
                vector[row_index] = cell.get<float>();
                ++row_index;
            }

            return vector;

        }

    } else if (json.is_number_float()) {
        return json.get<float>();
    }

    throw std::runtime_error("Unknown uniform type");
}

struct get_data_pointer_functor {
    std::pair<const void*, unsigned> operator()(const float& value) const {
        return { &value, 4 };
    }
    std::pair<const void*, unsigned> operator()(const glm::vec4& value) const {
        return { glm::value_ptr(value), 4 * 4 };
    }
    std::pair<const void*, unsigned> operator()(const glm::mat4& value) const {
        return { glm::value_ptr(value), 16 * 4 };
    }
};

std::pair<const void*, unsigned> get_data_pointer(
    const uniform_value &value
) {
    return std::visit(get_data_pointer_functor{}, value);
}

document from_file(const char* file_name) {
    std::ifstream i(file_name);
    nlohmann::json j;
    i >> j;

    document d;


    auto& actions = j.at("view_actions");
    d.view_actions.resize(actions.size());

    auto json_call = actions.begin();
    auto call = d.view_actions.begin();

    for (auto& json_call : actions) {
        auto type = json_call.at("type").get<std::string>();

        if (type == "program") {
            auto action = std::make_unique<program_action>();

            auto& shaders = json_call.at("shaders");

            action->vertex_shader = shaders.at("vertex").get<std::string>();
            action->fragment_shader = shaders.at("fragment").get<std::string>();
            action->viewport_width = 1280;
            action->viewport_height = 720;
            action->vertex_count =
                json_call.at("vertex_count").get<int>();
            action->out.push_back({"outColor", 0});

            auto json_uniforms = json_call.find("uniforms");
            if (json_uniforms != json_call.end()) {
                action->uniforms.resize(json_uniforms->size());

                // object iteration not supported with this version of nlohmann
                auto uniform = action->uniforms.begin();
                auto json_uniform = json_uniforms->begin();
                for (
                    ; json_uniform != json_uniforms->end();
                    ++json_uniform, ++uniform
                ) {
                    uniform->first = json_uniform.key();
                    uniform->second = from_json(json_uniform.value());
                }
            }

            *call = std::move(action);

        } else if (type == "blit") {
            auto action = std::make_unique<blit_action>();



            *call = std::move(action);

        } else {
            throw std::runtime_error("unsupported type");
        }

        ++call;
    }

    return d;
}
