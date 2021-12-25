#include <iostream>
#include <cassert>
#include <memory>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#define GLFW_VULKAN_STATIC
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "data/document.h"
#include "rendering/document.h"
#include "rendering/resources.h"
#include "rendering/renderer.h"

using namespace std;

VkSampleCountFlagBits max_sample_count;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*
) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        cerr << "validation layer error: " << callbackData->pMessage << endl;
        // ignore error caused by Nsight
        if (strcmp(callbackData->pMessageIdName, "Loader Message") != 0)
            throw runtime_error("vulkan error");
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        cout << "validation layer warning: " << callbackData->pMessage << endl;
    }

    return VK_FALSE;
}

struct view {
    view() = default;
    view(
        unsigned width, unsigned heigh, renderer& renderer, document& document,
        VkDevice device, VkPhysicalDevice physical_device,
        VkCommandPool command_pool,
        uint32_t graphics_queue_family, uint32_t present_queue_family,
        VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format
    );

    unsigned image_count;
    VkSurfaceCapabilitiesKHR capabilities;
    VkExtent2D extent;
    VkViewport viewport;
    VkRect2D scissors;
    unique_swapchain swapchain;
    std::vector<render_document> documents;
};

view::view(
    unsigned width, unsigned height, renderer& renderer, document& document,
    VkDevice device, VkPhysicalDevice physical_device,
    VkCommandPool command_pool,
    uint32_t graphics_queue_family, uint32_t present_queue_family,
    VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format
) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device, surface, &capabilities
    );
    auto present_mode = VK_PRESENT_MODE_FIFO_KHR;

    extent = {
        max(
            min<uint32_t>(width, capabilities.maxImageExtent.width),
            capabilities.minImageExtent.width
        ),
        max(
            min<uint32_t>(height, capabilities.maxImageExtent.height),
            capabilities.minImageExtent.height
        )
    };

    {
        uint32_t queue_family_indices[]{
            graphics_queue_family, present_queue_family
        };
        VkSwapchainCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = capabilities.minImageCount,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = size(queue_family_indices),
            .pQueueFamilyIndices = queue_family_indices,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };
        check(vkCreateSwapchainKHR(
            device, &create_info, nullptr, out_ptr(swapchain)
        ));
    }

    // read out actual number of swapchain images
    vkGetSwapchainImagesKHR(
        device, swapchain.get(), &image_count, nullptr
    );

    auto images = std::make_unique<VkImage[]>(image_count);
    vkGetSwapchainImagesKHR(
        device, swapchain.get(), &image_count, images.get()
    );

    documents.reserve(image_count);

    for (auto i = 0u; i < image_count; ++i) {
        documents.push_back(render_document(
            width, height, document, renderer, images[i],
            surface_format.format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            command_pool
        ));
    }
}

int main() {
    glfwInit();

    unsigned initial_window_width = 1280, initial_window_height = 720;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(
        initial_window_width, initial_window_height, "Vulkan", nullptr, nullptr
    );

    // set up error handling
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = nullptr
    };

    VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    // look up extensions needed by GLFW
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    // loop up supported extensions
    uint32_t supported_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(
        nullptr, &supported_extension_count, nullptr
    );
    auto supported_extensions =
        make_unique<VkExtensionProperties[]>(supported_extension_count);
    vkEnumerateInstanceExtensionProperties(
        nullptr, &supported_extension_count, supported_extensions.get()
    );

    // check support for layers
    std::initializer_list<const char*> enabled_layers = {
#ifdef EDITOR_VULKAN_VALIDATION
        "VK_LAYER_KHRONOS_validation",
#endif
    };

    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    auto layers = make_unique<VkLayerProperties[]>(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layers.get());
    for (const char* enabled_layer : enabled_layers) {
        bool supported = false;
        for (auto i = 0u; i < layer_count; i++) {
            auto equal = strcmp(enabled_layer, layers[i].layerName);
            if (equal == 0) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            throw runtime_error("enabled layer not supported");
        }
    }

    // create instance
    std::initializer_list<const char*> requiredExtensions = {
#ifdef EDITOR_VULKAN_VALIDATION
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
    auto extensionCount = size(requiredExtensions) + glfw_extension_count;
    auto extensions = make_unique<const char*[]>(extensionCount);
    std::copy(
        glfw_extensions, glfw_extensions + glfw_extension_count,
        extensions.get()
    );
    std::copy(
        requiredExtensions.begin(), requiredExtensions.end(),
        extensions.get() + glfw_extension_count
    );
    VkInstance instance;
    {
        VkInstanceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = &debugUtilsMessengerCreateInfo,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<uint32_t>(enabled_layers.size()),
            .ppEnabledLayerNames = enabled_layers.begin(),
            .enabledExtensionCount = static_cast<uint32_t>(extensionCount),
            .ppEnabledExtensionNames = extensions.get(),
        };
        check(vkCreateInstance(&createInfo, nullptr, &instance));
    }

    // create debug utils messenger
#ifdef EDITOR_VULKAN_VALIDATION
    auto vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT"
    );
    auto vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT"
    );
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug_callback,
        };
        check(vkCreateDebugUtilsMessengerEXT(
            instance, &createInfo, nullptr, &debugUtilsMessenger
        ));
    }
#endif

    // create surface
    VkSurfaceKHR surface;
    check(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    // look for available devices
    VkPhysicalDevice physical_device;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw runtime_error("no Vulkan capable GPU found");
    }
    {
        auto devices = make_unique<VkPhysicalDevice[]>(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.get());
        // TODO: check for VK_KHR_swapchain support
        physical_device = devices[0]; // just pick the first one for now
    }

    // get properties of physical device
    max_sample_count = VK_SAMPLE_COUNT_1_BIT;
    {
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(
            physical_device, &physical_device_properties
        );

        VkSampleCountFlags sample_count_falgs =
            physical_device_properties.limits.framebufferColorSampleCounts &
            physical_device_properties.limits.framebufferDepthSampleCounts &
            physical_device_properties.limits.framebufferStencilSampleCounts;

        for (auto bit : {
            VK_SAMPLE_COUNT_64_BIT, VK_SAMPLE_COUNT_32_BIT,
            VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_8_BIT,
            VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_2_BIT,
        }) {
            if (sample_count_falgs & bit) {
                max_sample_count = bit;
                break;
            }
        }
    }
    assert(max_sample_count != VK_SAMPLE_COUNT_1_BIT);
    std::cout << max_sample_count << std::endl;

    // look for available queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queueFamilyCount, nullptr
    );
    auto queueFamilies =
        make_unique<VkQueueFamilyProperties[]>(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queueFamilyCount, queueFamilies.get()
    );

    uint32_t graphics_queue_family = -1u, present_queue_family = -1u;
    for (auto i = 0u; i < queueFamilyCount; i++) {
        const auto& queueFamily = queueFamilies[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_family = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            physical_device, i, surface, &presentSupport
        );
        if (presentSupport) {
            present_queue_family = i;
        }
    }
    if (graphics_queue_family == -1u) {
        throw runtime_error("no suitable queue found");
    }

    // create queues and logical device
    // TODO: move VkDevice into renderer
    // maybe move VkPhysicalDevice too
    // on VK_ERROR_DEVICE_LOST try to recreate the VkDevice
    // if that fails try to look for another VkPhysicalDevice
    VkDevice device;
    current_device = &device;
    {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfos[]{
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphics_queue_family,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }, {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = present_queue_family,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }
        };

        const char* enabledExtensionNames[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = size(queueCreateInfos),
            .pQueueCreateInfos = queueCreateInfos,
            .enabledExtensionCount = size(enabledExtensionNames),
            .ppEnabledExtensionNames = enabledExtensionNames,
            .pEnabledFeatures = &deviceFeatures
        };

        check(vkCreateDevice(physical_device, &createInfo, nullptr, &device));
    }

    // retreive queues
    VkQueue graphicsQueue, presentQueue;
    vkGetDeviceQueue(device, graphics_queue_family, 0, &graphicsQueue);
    vkGetDeviceQueue(device, present_queue_family, 0, &presentQueue);

    // create swap chain
    uint32_t formatCount = 0, presentModeCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &formatCount, nullptr
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &presentModeCount, nullptr
    );
    if (formatCount == 0) {
        throw runtime_error("no surface formats supported");
    }
    if (presentModeCount == 0) {
        throw runtime_error("no surface present modes supported");
    }
    auto formats = make_unique<VkSurfaceFormatKHR[]>(formatCount);
    auto presentModes = make_unique<VkPresentModeKHR[]>(presentModeCount);

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &formatCount, formats.get()
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &presentModeCount, presentModes.get()
    );

    auto surface_format = formats[0];
    for (auto i = 0u; i < formatCount; i++) {
        auto format = formats[i];
        if (
            format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            surface_format = format;
        }
    }

    // create command pool
    VkCommandPool command_pool;
    {
        VkCommandPoolCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = graphics_queue_family,
        };
        check(vkCreateCommandPool(device, &createInfo, nullptr, &command_pool));
    }

    std::string document_file_name = "examples/example.json";

    document document = from_file(document_file_name.c_str());

    renderer renderer;
    renderer.graphics_queue_family = graphics_queue_family;
    renderer.present_queue_family = present_queue_family;
    vkGetPhysicalDeviceMemoryProperties(
        physical_device, &renderer.physical_device_memory_properties
    );

    {
        int framebuffer_width, framebuffer_height;
        glfwGetFramebufferSize(
            window, &framebuffer_width, &framebuffer_height
        );
        view view = {
            static_cast<unsigned int>(framebuffer_width),
            static_cast<unsigned int>(framebuffer_height),
            renderer, document,
            device, physical_device,
            command_pool, graphics_queue_family, present_queue_family,
            surface, surface_format
        };

        unique_semaphore swapchain_image_ready_semaphore;
        {
            VkSemaphoreCreateInfo semaphore_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            };
            vkCreateSemaphore(
                device, &semaphore_info, nullptr,
                out_ptr(swapchain_image_ready_semaphore)
            );
        }

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // get next image from swapchain
            uint32_t image_index;
            auto result = vkAcquireNextImageKHR(
                device, view.swapchain.get(), -1ul,
                swapchain_image_ready_semaphore.get(),
                VK_NULL_HANDLE,
                &image_index
            );

            if (result == VK_SUCCESS) {
                auto& document = view.documents[image_index];
                VkFence fence = document.fence.get();
                vkWaitForFences(device, 1, &fence, VK_TRUE, -1ul);
                vkResetFences(device, 1, &fence);

                // submit command buffer
                VkPipelineStageFlags wait_stage =
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                VkSemaphore render_finished_semaphore =
                    document.render_finished_semaphore.get();
                VkSemaphore wait_semaphore =
                    swapchain_image_ready_semaphore.get();
                VkSubmitInfo submitInfo = {
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = &wait_semaphore,
                    .pWaitDstStageMask = &wait_stage,
                    .commandBufferCount = 1,
                    .pCommandBuffers = &document.command_buffer,
                    .signalSemaphoreCount = 1,
                    .pSignalSemaphores = &render_finished_semaphore,
                };
                check(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence));

                auto swapchain = view.swapchain.get();

                // present image
                VkPresentInfoKHR presentInfo{
                    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = &render_finished_semaphore,
                    .swapchainCount = 1,
                    .pSwapchains = &swapchain,
                    .pImageIndices = &image_index,
                };
                vkQueuePresentKHR(presentQueue, &presentInfo);

            } else if (
                result == VK_SUBOPTIMAL_KHR ||
                result == VK_ERROR_OUT_OF_DATE_KHR
            ) {
                for (auto& document : view.documents) {
                    auto fence = document.fence.get();
                    vkWaitForFences(device, 1, &fence, VK_TRUE, -1ul);
                }

                int framebuffer_width, framebuffer_height;
                glfwGetFramebufferSize(
                    window, &framebuffer_width, &framebuffer_height
                );
                if (framebuffer_height > 0 && framebuffer_width > 0) {
                    // destroy view first
                    {
                        ::view old;
                        std::swap(old, view);
                    }
                    view = {
                        static_cast<unsigned int>(framebuffer_width),
                        static_cast<unsigned int>(framebuffer_height),
                        renderer, document,
                        device, physical_device,
                        command_pool, graphics_queue_family,
                        present_queue_family,
                        surface, surface_format
                    };
                }

            } else {
                check(result);
            }

            // TODO: swapchain doesn't necessarily sync with current monitor
            // use VK_KHR_display to wait for vsync of current display
        }

        // TODO: destructors don't wait on exception
        for(auto& document : view.documents) {
            auto fence = document.fence.get();
            vkWaitForFences(device, 1, &fence, VK_TRUE, -1u);
        }
    }

    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
#ifdef EDITOR_VULKAN_VALIDATION
    vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
#endif
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
