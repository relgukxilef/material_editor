#include "shader.h"

#include <fstream>
#include <vector>
#include <cinttypes>
#include <exception>
#include <algorithm>

#include <shaderc/shaderc.hpp>
#include "spirv_reflect.h"

reflected_shader_module::reflected_shader_module(
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

    SpvReflectShaderModule reflect_shader;
    if (
        spvReflectCreateShaderModule(
            binary.size() * 4, binary.data(), &reflect_shader
        ) != SPV_REFLECT_RESULT_SUCCESS
    ) {
        throw std::runtime_error(
            "Failed to read shader binary after compilation"
        );
    }

    uint32_t descriptor_set_count;
    spvReflectEnumerateDescriptorSets(
        &reflect_shader, &descriptor_set_count, nullptr
    );

    std::vector<SpvReflectDescriptorSet*> reflect_descriptor_sets(
        descriptor_set_count
    );
    spvReflectEnumerateDescriptorSets(
        &reflect_shader, &descriptor_set_count, reflect_descriptor_sets.data()
    );

    descriptor_size = 0;

    for (auto descriptor_set : reflect_descriptor_sets) {
        for (auto i = 0u; i < descriptor_set->binding_count; i++) {
            auto binding = descriptor_set->bindings[i];

            if (
                binding->descriptor_type ==
                SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            ) {
                auto &block = binding->block;
                for (auto j = 0u; j < block.member_count; j++) {
                    auto &member = block.members[j];
                    descriptor_offsets.insert({
                        member.name, member.absolute_offset
                    });
                    descriptor_size = std::max(
                        descriptor_size,
                        member.absolute_offset + member.padded_size
                    );
                }
            }
        }
    }

    spvReflectDestroyShaderModule(&reflect_shader);

    VkShaderModuleCreateInfo shader_info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = binary.size() * 4,
        .pCode = binary.data()
    };

    check(vkCreateShaderModule(device, &shader_info, nullptr, out_ptr(module)));
}
