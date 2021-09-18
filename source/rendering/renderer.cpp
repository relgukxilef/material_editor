#include "renderer.h"

#include <fstream>
#include <vector>
#include <cinttypes>
#include <exception>
#include <algorithm>

#include <shaderc/shaderc.hpp>

renderer::renderer() {
    compiler_options.SetOptimizationLevel(
        shaderc_optimization_level_performance
    );
}

VkShaderModule create_shader_from_source(
    const renderer &renderer, VkDevice device, const char *file_name,
    shaderc_shader_kind kind
) {
    std::ifstream file(file_name, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(
            std::string("Couldn't open file ") + file_name
        );
    }

    auto size = file.tellg();
    std::vector<char> source(size);

    file.seekg(0);
    file.read(source.data(), size);

    auto compilation = renderer.compiler.CompileGlslToSpv(
        source.data(), source.size(), kind,
        file_name, renderer.compiler_options
    );

    if (
        compilation.GetCompilationStatus() != shaderc_compilation_status_success
    ) {
        throw std::runtime_error(compilation.GetErrorMessage());
    }

    std::vector<uint32_t> binary(compilation.begin(), compilation.end());

    VkShaderModuleCreateInfo shader_info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = binary.size() * 4,
        .pCode = binary.data()
    };

    VkShaderModule shader;
    if (
        vkCreateShaderModule(device, &shader_info, nullptr, &shader) !=
        VK_SUCCESS
    ) {
        throw std::runtime_error("Couldn't create shader");
    }

    return shader;
}
