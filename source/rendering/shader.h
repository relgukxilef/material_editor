#pragma once

#include <unordered_map>
#include <string>

#include "resources.h"
#include "renderer.h"

struct reflected_shader_module {
    reflected_shader_module(
        const renderer &renderer, VkDevice device, const char *file_name,
        shaderc_shader_kind kind
    );

    unique_shader_module module;
    std::unordered_map<std::string, uint32_t> descriptor_offsets;
    unsigned descriptor_size;
};
