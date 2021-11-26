#pragma once

#include <unordered_map>
#include <string>

#include "resources.h"

struct reflected_shader_module {
    unique_shader_module module;
    std::unordered_map<std::string, uint32_t> descriptor_offsets;
    unsigned descriptor_size;
};

