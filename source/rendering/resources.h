#pragma once

#include <vulkan/vulkan.h>

extern VkDevice *current_device;

template<typename T, auto Deleter>
struct unique_vulkan_resource {
    typedef T pointer;

    unique_vulkan_resource() : value(VK_NULL_HANDLE) {}
    unique_vulkan_resource(T value) : value(value) {}
    unique_vulkan_resource(const unique_vulkan_resource&) = delete;
    unique_vulkan_resource(unique_vulkan_resource &&o) : value(o.value) {
        o.value = VK_NULL_HANDLE;
    }
    ~unique_vulkan_resource() {
        if (value != VK_NULL_HANDLE)
            Deleter(*current_device, value, nullptr);
    }
    unique_vulkan_resource& operator= (const unique_vulkan_resource&) = delete;
    unique_vulkan_resource& operator= (unique_vulkan_resource &&o) {
        if (value != VK_NULL_HANDLE)
            Deleter(*current_device, value, nullptr);
        value = o.value;
        o.value = VK_NULL_HANDLE;
        return *this;
    }

    const T& get() {
        return value;
    };

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

typedef unique_vulkan_resource<
    VkDescriptorSetLayout, vkDestroyDescriptorSetLayout
> unique_descriptor_set_layout;

typedef unique_vulkan_resource<VkRenderPass, vkDestroyRenderPass>
    unique_render_pass;

typedef unique_vulkan_resource<VkCommandPool, vkDestroyCommandPool>
    unique_command_pool;

typedef unique_vulkan_resource<VkFence, vkDestroyFence> unique_fence;

typedef unique_vulkan_resource<VkSemaphore, vkDestroySemaphore>
    unique_semaphore;

typedef unique_vulkan_resource<VkSwapchainKHR, vkDestroySwapchainKHR>
    unique_swapchain;

typedef unique_vulkan_resource<VkBuffer, vkDestroyBuffer>
    unique_buffer;

typedef unique_vulkan_resource<VkDeviceMemory, vkFreeMemory>
    unique_device_memory;

typedef unique_vulkan_resource<VkDescriptorPool, vkDestroyDescriptorPool>
    unique_descriptor_pool;


template<class Smart, class Pointer>
struct out_ptr_t {
    out_ptr_t(Smart &smart) : smart(smart) {}
    ~out_ptr_t() { smart = Smart(pointer); }

    operator Pointer*() { return &pointer; }

    Smart &smart;
    Pointer pointer;
};

template<class Smart>
out_ptr_t<Smart, typename Smart::pointer> out_ptr(Smart &smart) {
    return { smart };
}
