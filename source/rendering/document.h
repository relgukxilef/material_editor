#pragma once

#include <vector>

#include "../data/document.h"
#include "resources.h"
#include "renderer.h"

struct render_action {
    unique_pipeline_layout pipeline_layout;
    unique_pipeline pipeline;
    unique_framebuffer framebuffer;
    unique_render_pass render_pass;
    // framebuffer and render_pass are resolution dependent
    // need one per action (or caching/partitioning)
    unsigned width, height;
};

struct render_texture {
    unique_image image;
    unique_image_view view;
    unsigned width, height;
};

struct render_document {
    // is dependent on swapchain image and re-created on resolution changes
    render_document(
        unsigned width, unsigned height,
        const document& document, const renderer &renderer, VkImage output,
        VkFormat output_format, VkImageLayout output_layout,
        VkCommandPool graphics_command_pool
    );

    unsigned width, height;
    // TODO: can't put swapchain image in this vector
    std::vector<render_texture> textures;
    std::vector<render_action> view_actions;
    VkCommandBuffer command_buffer;

    unique_fence fence;
    unique_image_view output_view;
    unique_semaphore render_finished_semaphore;
};


