#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

struct renderer {
    renderer();

    shaderc::Compiler compiler;
    shaderc::CompileOptions compiler_options;
};

VkShaderModule create_shader_from_source(
    renderer& renderer, VkDevice device, const char* file_name
);
