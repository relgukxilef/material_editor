#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <variant>
#include <string_view>

#include <glm/glm.hpp>

enum built_in_textures : unsigned {
    built_in_window = 1024,
};

enum struct format {
    a2b10g10r10_unorm_pack32,
    a2b10g10r10_snorm_pack32,
    r8g8b8a8_unorm,
    b8g8r8a8_srgb,
};

enum struct texture_usage : unsigned {
    present,
    draw,
    compute,
};

struct texture_definition {
    unsigned width, height, depth;
    format format;
};

struct texture_binding {
    unsigned texture;
    texture_usage next_usage;
};

typedef std::variant<
    int, unsigned, float, glm::vec4, glm::mat4
> uniform_value;

struct action {
    std::vector<std::pair<std::string, uniform_value>> uniforms;

    std::string vertex_shader, fragment_shader;
    std::vector<std::pair<std::string, unsigned>> in;
    std::vector<std::pair<std::string, texture_binding>> out;
    unsigned viewport_width, viewport_height;
};

struct document {
    std::unordered_map<std::string, texture_definition> textures;
    std::vector<action> view_actions;

    unsigned display_texture;
};

document from_file(const char* file_name);
