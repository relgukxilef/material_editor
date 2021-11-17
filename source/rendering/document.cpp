#include "document.h"

render_document::render_document(
    unsigned width, unsigned height,
    const document &document, const renderer &renderer,
    VkImage output, VkFormat output_format, VkImageLayout output_layout,
    VkCommandPool graphics_command_pool

) : width(width), height(height), view_actions(document.view_actions.size()) {

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    vkCreateFence(
        *current_device, &fence_info, nullptr, out_ptr(fence)
    );
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    vkCreateSemaphore(
        *current_device, &semaphore_info, nullptr,
        out_ptr(render_finished_semaphore)
    );

    for (size_t i = 0; i < document.textures.size(); i++) {
        // TODO: create images and image views
    }

    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = graphics_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    if (
        vkAllocateCommandBuffers(
            *current_device, &command_buffer_info, &command_buffer
        ) != VK_SUCCESS
    ) {
        throw std::runtime_error("Failed to allocate command buffer");
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    VkImageViewCreateInfo image_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = output,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = output_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    vkCreateImageView(
        *current_device, &image_view_info, nullptr,
        out_ptr(this->output_view)
    );

    for (size_t i = 0; i < document.view_actions.size(); i++) {
        // TODO: multi-thread shader compilation
        // TODO: multi-thread pipeline compilation
        auto& action = document.view_actions[i];

        unique_shader_module vertex_shader = create_shader_from_source(
            renderer, *current_device,
            ("examples/" + action.vertex_shader).c_str(), // TODO
            shaderc_glsl_vertex_shader
        );
        unique_shader_module fragment_shader = create_shader_from_source(
            renderer, *current_device,
            ("examples/" + action.fragment_shader).c_str(), // TODO
            shaderc_glsl_fragment_shader
        );

        VkPipelineShaderStageCreateInfo pipeline_shader_stage_info[] = {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertex_shader.get(),
                .pName = "main",
            }, {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragment_shader.get(),
                .pName = "main",
            },
        };
        VkPipelineVertexInputStateCreateInfo vertex_input_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };
        VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
            .sType =
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };
        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(width), // TODO
            .height = static_cast<float>(height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        VkRect2D scissors = {
            .offset = {0, 0},
            .extent = {width, height},
        };
        VkPipelineViewportStateCreateInfo viewport_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissors,
        };
        VkPipelineRasterizationStateCreateInfo rasterization_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f,
        };
        VkPipelineMultisampleStateCreateInfo multisample_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
        };
        VkPipelineColorBlendAttachmentState color_blend_attachment = {
            .blendEnable = VK_FALSE,
            .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT,
        };
        VkPipelineColorBlendStateCreateInfo color_blend_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &color_blend_attachment,
        };
        VkPipelineLayoutCreateInfo pipeline_layout_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        };
        unique_pipeline_layout pipeline_layout;
        if (
            vkCreatePipelineLayout(
                *current_device, &pipeline_layout_info, nullptr,
                out_ptr(pipeline_layout)
            ) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create pipeline layout");
        }

        auto attachments =
            std::make_unique<VkAttachmentDescription[]>(action.out.size());
        auto attachment_references =
            std::make_unique<VkAttachmentReference[]>(action.out.size());

        {
            int i = 0;
            for (auto& [name, texture] : action.out) {
                VkImageLayout final_layout;
                if (texture.next_usage == texture_usage::present) {
                    final_layout = output_layout;
                } else if (texture.next_usage == texture_usage::draw) {
                    final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                } else {
                    final_layout = VK_IMAGE_LAYOUT_GENERAL;
                }

                attachments[i] = VkAttachmentDescription{
                    .format = VK_FORMAT_A2B10G10R10_UNORM_PACK32,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .finalLayout = final_layout,
                };

                attachment_references[i] = {
                    .attachment = static_cast<uint32_t>(i),
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };

                i++;
            }
        }

        // TODO: some actions could share render passes
        VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, // TODO
            .colorAttachmentCount = static_cast<uint32_t>(action.out.size()),
            .pColorAttachments = attachment_references.get()
        };
        VkRenderPassCreateInfo render_pass_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(action.out.size()),
            .pAttachments = attachments.get(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
        };
        unique_render_pass render_pass;
        if (
            vkCreateRenderPass(
                *current_device, &render_pass_info, nullptr,
                out_ptr(render_pass)
            ) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create render pass");
        }

        VkGraphicsPipelineCreateInfo pipeline_info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = std::size(pipeline_shader_stage_info),
            .pStages = pipeline_shader_stage_info,
            .pVertexInputState = &vertex_input_info,
            .pInputAssemblyState = &input_assembly_info,
            .pViewportState = &viewport_info,
            .pRasterizationState = &rasterization_info,
            .pMultisampleState = &multisample_info,
            .pColorBlendState = &color_blend_info,
            .layout = pipeline_layout.get(),
            .renderPass = render_pass.get(),
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        };
        unique_pipeline pipeline;
        if (
            vkCreateGraphicsPipelines(
                *current_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                out_ptr(pipeline)
            ) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create pipeline");
        }

        // TODO: some actions could share framebuffer
        VkImageView output_view = this->output_view.get();
        VkFramebufferCreateInfo framebuffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass.get(),
            .attachmentCount = static_cast<uint32_t>(action.out.size()),
            .pAttachments = &output_view,
            .width = width,
            .height = height,
            .layers = 1,
        };
        unique_framebuffer framebuffer;
        if (
            vkCreateFramebuffer(
                *current_device, &framebuffer_info, nullptr,
                out_ptr(framebuffer)
            ) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create framebuffer");
        }

        VkClearValue clear_value = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
        VkRenderPassBeginInfo render_pass_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass.get(),
            .framebuffer = framebuffer.get(),
            .renderArea = {
                .offset = {0, 0},
                .extent = {width, height},
            },
            .clearValueCount = 1,
            .pClearValues = &clear_value,
        };
        vkCmdBeginRenderPass(
            command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE
        );
        vkCmdBindPipeline(
            command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get()
        );
        vkCmdDraw(command_buffer, 3, 1, 0, 0); // TODO
        vkCmdEndRenderPass(command_buffer);

        view_actions[i] = {
            .pipeline_layout = std::move(pipeline_layout),
            .pipeline = std::move(pipeline),
            .framebuffer = std::move(framebuffer),
            .render_pass = std::move(render_pass),
        };
    }

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }
}
