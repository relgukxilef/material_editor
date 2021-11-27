#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <variant>
#include <memory>

#include <glm/glm.hpp>

enum built_in_textures : unsigned {
    built_in_window = 1024,
};

enum built_in_variables : unsigned {
    window_width, window_height,
    view_matrix, projection_matrix,
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

typedef std::variant<
    float, glm::vec4, glm::mat4
> uniform_value;

std::pair<const void*, unsigned> get_data_pointer(const uniform_value& value);

struct program_action {
    std::vector<std::pair<std::string, uniform_value>> uniforms;

    std::string vertex_shader, fragment_shader;
    std::vector<std::pair<std::string, unsigned>> in;
    std::vector<std::pair<std::string, unsigned>> out;
    unsigned viewport_width, viewport_height;
    unsigned vertex_count;
};

struct blit_action {
    unsigned source_texture;
    unsigned destination_texture;
};

typedef std::variant<
    std::unique_ptr<program_action>, std::unique_ptr<blit_action>
> action;

struct document {
    std::unordered_map<std::string, texture_definition> textures;
    std::vector<action> view_actions;

    unsigned display_texture;
};

document from_file(const char* file_name);
