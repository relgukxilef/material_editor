#pragma once

#include <vulkan/vulkan.h>

extern VkDevice *current_device;

template<typename T, auto Deleter>
struct unique_vulkan_resource {
    unique_vulkan_resource() : value(VK_NULL_HANDLE) {}
    unique_vulkan_resource(T value) : value(value) {}
    unique_vulkan_resource(const unique_vulkan_resource&) = delete;
    unique_vulkan_resource(unique_vulkan_resource&& o) : value(o.value) {
        o.value = VK_NULL_HANDLE;
    }
    ~unique_vulkan_resource() {
        if (value != VK_NULL_HANDLE)
            Deleter(*current_device, value, nullptr);
    }
    unique_vulkan_resource& operator= (const unique_vulkan_resource&) = delete;
    unique_vulkan_resource& operator= (unique_vulkan_resource&& o) {
        if (value != VK_NULL_HANDLE)
            Deleter(*current_device, value, nullptr);
        value = o.value;
        o.value = VK_NULL_HANDLE;
        return *this;
    }

    T get() {
        return value;
    };

    T& initialize_into() {
        if (value != VK_NULL_HANDLE)
            Deleter(*current_device, value, nullptr);
        return value;
    }

private:
    T value;
};

typedef unique_vulkan_resource<VkShaderModule, vkDestroyShaderModule>
    unique_shader_module;

typedef unique_vulkan_resource<VkFramebuffer, vkDestroyFramebuffer>
    unique_framebuffer;

typedef unique_vulkan_resource<VkPipeline, vkDestroyPipeline>
    unique_pipeline;

typedef unique_vulkan_resource<VkImage, vkDestroyImage> unique_image;

typedef unique_vulkan_resource<VkImageView, vkDestroyImageView>
    unique_image_view;

typedef unique_vulkan_resource<VkPipelineLayout, vkDestroyPipelineLayout>
    unique_pipeline_layout;

typedef unique_vulkan_resource<VkRenderPass, vkDestroyRenderPass>
    unique_render_pass;

typedef unique_vulkan_resource<VkCommandPool, vkDestroyCommandPool>
    unique_command_pool;

typedef unique_vulkan_resource<VkFence, vkDestroyFence> unique_fence;

typedef unique_vulkan_resource<VkSemaphore, vkDestroySemaphore>
    unique_semaphore;

typedef unique_vulkan_resource<VkSwapchainKHR, vkDestroySwapchainKHR>
    unique_swapchain;
