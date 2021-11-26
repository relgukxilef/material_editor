#include "document.h"

struct compile_action_functor {
    const renderer &renderer;
    render_document &document;
    VkImageLayout output_layout;

    void operator() (const std::unique_ptr<program_action> &action_pointer) {
        auto &action = *action_pointer;

        reflected_shader_module vertex_shader = create_shader_from_source(
            renderer, *current_device,
            ("examples/" + action.vertex_shader).c_str(), // TODO
            shaderc_glsl_vertex_shader
        );
        reflected_shader_module fragment_shader = create_shader_from_source(
            renderer, *current_device,
            ("examples/" + action.fragment_shader).c_str(), // TODO
            shaderc_glsl_fragment_shader
        );

        VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
        };
        VkDescriptorSetLayoutCreateInfo descriptor_set_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &descriptor_set_layout_binding,
        };
        unique_descriptor_set_layout descriptor_set_layout;
        check(vkCreateDescriptorSetLayout(
            *current_device, &descriptor_set_info,
            nullptr, out_ptr(descriptor_set_layout)
        ));

        uint32_t queue_family_index = renderer.graphics_queue_family;
        VkBufferCreateInfo uniform_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = vertex_shader.descriptor_size,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &queue_family_index,
        };
        unique_buffer uniform_buffer;
        unique_device_memory uniform_memory;
        void* uniform_data;
        if (vertex_shader.descriptor_size > 0) {
            check(vkCreateBuffer(
                *current_device, &uniform_buffer_info, nullptr,
                out_ptr(uniform_buffer)
            ));
            VkMemoryRequirements memory_requirements;
            vkGetBufferMemoryRequirements(
                *current_device, uniform_buffer.get(), &memory_requirements
            );
            VkMemoryPropertyFlags properties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            uint32_t memory_type_index;
            for (
                memory_type_index = 0;
                memory_type_index <
                renderer.physical_device_memory_properties.memoryTypeCount;
                memory_type_index++
            ) {
                if (
                    (
                        memory_requirements.memoryTypeBits &
                        (1 << memory_type_index)
                    ) && (
                        renderer.physical_device_memory_properties.memoryTypes[
                            memory_type_index
                        ].propertyFlags &
                        properties
                    )
                )
                    break;
            }
            VkMemoryAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memory_requirements.size,
                .memoryTypeIndex = memory_type_index,
            };
            check(vkAllocateMemory(
                *current_device, &allocate_info, nullptr,
                out_ptr(uniform_memory)
            ));
            check(vkBindBufferMemory(
                *current_device, uniform_buffer.get(), uniform_memory.get(), 0
            ));

            vkMapMemory(
                *current_device, uniform_memory.get(), 0,
                vertex_shader.descriptor_size, 0, &uniform_data
            );
            std::array<float, 4> color = {0.5, 1.0, 1.0, 0.0};
            memcpy(uniform_data, color.begin(), sizeof(color));
        }

        VkDescriptorPoolSize pool_size = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1, // TODO: share between swap chain images
        };
        VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,  // TODO: share between swap chain images
            .poolSizeCount = 1,
            .pPoolSizes = &pool_size,
        };
        unique_descriptor_pool descriptor_pool;
        check(vkCreateDescriptorPool(
            *current_device, &pool_info, nullptr, out_ptr(descriptor_pool))
        );

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descriptor_pool.get(),
            .descriptorSetCount = 1, // TODO: share between swap chain images
            .pSetLayouts = &descriptor_set_layout.get(),
        };
        VkDescriptorSet descriptor_set;
        check(vkAllocateDescriptorSets(
            *current_device, &descriptor_set_allocate_info, &descriptor_set
        ));

        VkDescriptorBufferInfo descriptor_buffer_info = {
            .buffer = uniform_buffer.get(),
            .offset = 0,
            .range = vertex_shader.descriptor_size,
        };
        VkWriteDescriptorSet write_descriptor_set = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &descriptor_buffer_info,
        };
        vkUpdateDescriptorSets(
            *current_device, 1, &write_descriptor_set, 0, nullptr
        );

        VkPipelineShaderStageCreateInfo pipeline_shader_stage_info[] = {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertex_shader.module.get(),
                .pName = "main",
            }, {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragment_shader.module.get(),
                .pName = "main",
            },
        };
        VkPipelineVertexInputStateCreateInfo vertex_input_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };
        VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
            .sType =
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE,
        };
        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(document.width), // TODO
            .height = static_cast<float>(document.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        VkRect2D scissors = {
            .offset = {0, 0},
            .extent = {document.width, document.height},
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
        VkDescriptorSetLayout layouts[] = { descriptor_set_layout.get() };
        VkPipelineLayoutCreateInfo pipeline_layout_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = layouts,
        };
        unique_pipeline_layout pipeline_layout;
        check(vkCreatePipelineLayout(
            *current_device, &pipeline_layout_info, nullptr,
            out_ptr(pipeline_layout)
        ));

        auto attachments =
            std::make_unique<VkAttachmentDescription[]>(action.out.size());
        auto attachment_references =
            std::make_unique<VkAttachmentReference[]>(action.out.size());

        {
            int i = 0;
            for (auto& out : action.out) {
                (void)out;
                VkImageLayout final_layout;
                final_layout = output_layout;

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
        check(vkCreateRenderPass(
            *current_device, &render_pass_info, nullptr,
            out_ptr(render_pass)
        ));

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
        check(vkCreateGraphicsPipelines(
            *current_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
            out_ptr(pipeline)
        ));

        // TODO: some actions could share framebuffer
        VkImageView output_view = document.output_view.get();
        VkFramebufferCreateInfo framebuffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass.get(),
            .attachmentCount = static_cast<uint32_t>(action.out.size()),
            .pAttachments = &output_view,
            .width = document.width,
            .height = document.height,
            .layers = 1,
        };
        unique_framebuffer framebuffer;
        check(vkCreateFramebuffer(
            *current_device, &framebuffer_info, nullptr,
            out_ptr(framebuffer)
        ));

        VkClearValue clear_value = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
        VkRenderPassBeginInfo render_pass_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass.get(),
            .framebuffer = framebuffer.get(),
            .renderArea = {
                .offset = {0, 0},
                .extent = {document.width, document.height},
            },
            .clearValueCount = 1,
            .pClearValues = &clear_value,
        };
        vkCmdBeginRenderPass(
            document.command_buffer, &render_pass_begin_info,
            VK_SUBPASS_CONTENTS_INLINE
        );
        vkCmdBindPipeline(
            document.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.get()
        );
        vkCmdBindDescriptorSets(
            document.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout.get(), 0, 1, &descriptor_set, 0, nullptr
        );
        vkCmdDraw(document.command_buffer, action.vertex_count, 1, 0, 0);
        vkCmdEndRenderPass(document.command_buffer);

        document.render_program_actions.push_back(render_program_action{
            .pipeline_layout = std::move(pipeline_layout),
            .pipeline = std::move(pipeline),
            .framebuffer = std::move(framebuffer),
            .render_pass = std::move(render_pass),
            .uniform_buffer = std::move(uniform_buffer),
            .uniform_memory = std::move(uniform_memory),
            .descriptor_pool = std::move(descriptor_pool),
            .uniform_data = uniform_data,
        });
    }
    void operator() (const std::unique_ptr<blit_action> &) {

    }
};

render_document::render_document(
    unsigned width, unsigned height,
    const document &document, const renderer &renderer,
    VkImage output, VkFormat output_format, VkImageLayout output_layout,
    VkCommandPool graphics_command_pool

) :
    width(width), height(height),
    render_program_actions(document.view_actions.size())
{

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
    check(vkAllocateCommandBuffers(
        *current_device, &command_buffer_info, &command_buffer
    ));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    check(vkBeginCommandBuffer(command_buffer, &begin_info));

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

        std::visit(
            compile_action_functor{renderer, *this, output_layout},
            document.view_actions[i]
        );
    }

    check(vkEndCommandBuffer(command_buffer));
}
