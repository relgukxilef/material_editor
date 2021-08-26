#pragma once

#include <string>
#include <vector>
#include <utility>
#include <variant>
#include <string_view>

#include <glm/glm.hpp>

struct texture {
    std::string name;
    unsigned width, height, depth;
};

typedef std::variant<
    int, unsigned, float, glm::vec4, glm::mat4, std::string
> uniform_value;

struct call {
    std::vector<std::string> shader_modules;
    std::vector<std::pair<std::string, unsigned>> input_textures;
    std::vector<std::pair<std::string, unsigned>> output_textures;
    std::vector<std::pair<std::string, uniform_value>> uniforms;
};

struct document {
    document();

    std::vector<std::string> textures;
    std::vector<call> calls;

    unsigned display_texture;
};

document from_file(const char* file_name);
