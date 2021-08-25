#include <iostream>
#include <cassert>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#define GLFW_VULKAN_STATIC
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

VkSampleCountFlagBits max_sample_count;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*
) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        //throw runtime_error("vulkan error");
        // can't throw because Nsight causes errors I don't care about
        cerr << "validation layer error: " << callbackData->pMessage << endl;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        cout << "validation layer warning: " << callbackData->pMessage << endl;
    }

    return VK_FALSE;
}

int main() {
    glfwInit();

    unsigned windowWidth = 1280, windowHeight = 720;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window =
        glfwCreateWindow(windowWidth, windowHeight, "Vulkan", nullptr, nullptr);

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
    const char* enabled_layers[]{
        "VK_LAYER_KHRONOS_validation",
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
    const char *requiredExtensions[]{
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    auto extensionCount = size(requiredExtensions) + glfw_extension_count;
    auto extensions = make_unique<const char*[]>(extensionCount);
    std::copy(
        glfw_extensions, glfw_extensions + glfw_extension_count,
        extensions.get()
    );
    std::copy(
        requiredExtensions, requiredExtensions + size(requiredExtensions),
        extensions.get() + glfw_extension_count
    );
    VkInstance instance;
    {
        VkInstanceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = &debugUtilsMessengerCreateInfo,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = std::size(enabled_layers),
            .ppEnabledLayerNames = enabled_layers,
            .enabledExtensionCount = static_cast<uint32_t>(extensionCount),
            .ppEnabledExtensionNames = extensions.get(),
        };
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw runtime_error("failed to create instance");
        }
    }

    // create debug utils messenger
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
        if (
            vkCreateDebugUtilsMessengerEXT(
                instance, &createInfo, nullptr, &debugUtilsMessenger
            ) != VK_SUCCESS
        ) {
            throw runtime_error("failed to create debug utils messenger");
        }
    }

    // create surface
    VkSurfaceKHR surface;
    if (
        glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
        VK_SUCCESS
    ) {
        throw runtime_error("failed to create window surface");
    }

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

    uint32_t graphicsQueueFamily = -1u, presentQueueFamily = -1u;
    for (auto i = 0u; i < queueFamilyCount; i++) {
        const auto& queueFamily = queueFamilies[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            physical_device, i, surface, &presentSupport
        );
        if (presentSupport) {
            presentQueueFamily = i;
        }
    }
    if (graphicsQueueFamily == -1u) {
        throw runtime_error("no suitable queue found");
    }

    // create queues and logical device
    VkDevice device;
    {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfos[]{
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphicsQueueFamily,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }, {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = presentQueueFamily,
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

        if (
            vkCreateDevice(physical_device, &createInfo, nullptr, &device) !=
            VK_SUCCESS
        ) {
            throw runtime_error("failed to create logical device");
        }
    }

    // retreive queues
    VkQueue graphicsQueue, presentQueue;
    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);

    // create swap chains
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

    auto surfaceFormat = formats[0];
    for (auto i = 0u; i < formatCount; i++) {
        auto format = formats[i];
        if (
            format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            surfaceFormat = format;
        }
    }

    // create command pool
    VkCommandPool commandPool;
    {
        VkCommandPoolCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = graphicsQueueFamily,
        };
        if (
            vkCreateCommandPool(device, &createInfo, nullptr, &commandPool) !=
            VK_SUCCESS
        ) {
            throw runtime_error("failed to create command pool");
        }
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // TODO: swapchain doesn't necessarily sync with current monitor
        // use VK_KHR_display to wait for vsync of current display
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
