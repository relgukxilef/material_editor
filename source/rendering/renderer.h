#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include "resources.h"

struct renderer {
    renderer();

    shaderc::Compiler compiler;
    shaderc::CompileOptions compiler_options;
};

VkShaderModule create_shader_from_source(
    const renderer &renderer, VkDevice device, const char* file_name,
    shaderc_shader_kind kind
);
