#pragma once

#include <string>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include "resources.h"

struct renderer {
    renderer();

    // TODO: move compiler to application struct
    shaderc::Compiler compiler;
    shaderc::CompileOptions compiler_options;

    uint32_t graphics_queue_family, present_queue_family;
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
};

struct out_of_memory_error : public std::bad_alloc {
    out_of_memory_error() : std::bad_alloc() {}
};

struct device_lost : public std::runtime_error {
    device_lost() : std::runtime_error("Device lost") {}
};

inline void check(VkResult success) {
    if (success == VK_SUCCESS) {
        return;
    } else if (success == VK_ERROR_OUT_OF_HOST_MEMORY) {
        throw out_of_memory_error();
    } else if (success == VK_ERROR_DEVICE_LOST) {
        throw device_lost();
    } else {
        throw std::runtime_error("Unknown error");
    }
}
