#include <Engine/ufox_gui.hpp>
namespace ufox::gui {

    GUI::GUI(gpu::vulkan::GraphicsDevice& gpu)
        : _gpu(gpu) {

    }

    void GUI::addStyle(const std::string& name, const GUIStyle& style, uint32_t inFlight) {
        GUIStyleBuffer styleBuffer;
        styleBuffer.name = name;
        styleBuffer.content = style;
        styleBuffer.buffer.reserve(inFlight);
        styleBuffer.mapped.reserve(inFlight);
        for (uint32_t i = 0; i < inFlight; ++i) {
            gpu::vulkan::Buffer buffer{};

            _gpu.createBuffer(GUI_STYLE_BUFFER_SIZE, vk::BufferUsageFlagBits::eUniformBuffer,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                              buffer);
            if (!buffer.memory) {
                throw std::runtime_error("Buffer memory is not initialized for buffer " + std::to_string(i));
            }
            auto* mapped = static_cast<uint8_t*>(buffer.memory->mapMemory(0, GUI_STYLE_BUFFER_SIZE));
            std::memcpy(mapped, &styleBuffer.content, GUI_STYLE_BUFFER_SIZE);
            styleBuffer.buffer.emplace_back(std::move(buffer));
            styleBuffer.mapped.emplace_back(mapped);
        }
        _styleContainer.push_back(std::move(styleBuffer));
    }

    gpu::vulkan::Buffer* GUI::getStyleBuffer(uint32_t index, uint32_t frameIndex) {

        auto& b = _styleContainer[index];

        return &b.buffer[frameIndex];
    }

    void GUI::update() const {
        core::TransformMatrix ubo{};
        ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0+10, 0+10, 0.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(500, 500, 1));
        ubo.view = glm::mat4(1.0f);
        ubo.proj = glm::ortho(
        0.0f, static_cast<float>(_gpu.getSwapchainExtent().width),
        0.0f, static_cast<float>(_gpu.getSwapchainExtent().height), // Swap bottom and top
        -1.0f, 1.0f);

        for (auto ubm: uniformBuffersMapped) {
            memcpy(ubm, &ubo, core::TRANSFORM_BUFFER_SIZE);
        }

        core::TransformMatrix ubo2{};
        ubo2.model = glm::translate(glm::mat4(1.0f), glm::vec3(0+400, 0+10, 0.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(500, 500, 1));
        ubo2.view = glm::mat4(1.0f);
        ubo2.proj = glm::ortho(
        0.0f, static_cast<float>(_gpu.getSwapchainExtent().width),
        0.0f, static_cast<float>(_gpu.getSwapchainExtent().height), // Swap bottom and top
        -1.0f, 1.0f);

        for (auto ubm: uniformBuffersMapped2) {
            memcpy(ubm, &ubo2, core::TRANSFORM_BUFFER_SIZE);
        }
    }


    void GUI::draw(const vk::raii::CommandBuffer &cmd) const {
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphicsPipeline);

        cmd.setViewport(0, vk::Viewport{ 0.0f, 0.0f, static_cast<float>(_gpu.getSwapchainExtent().width), static_cast<float>(_gpu.getSwapchainExtent().height), 0.0f, 1.0f });
        cmd.setScissor(0, vk::Rect2D{ {0, 0}, _gpu.getSwapchainExtent() });
        cmd.setCullMode(vk::CullModeFlagBits::eNone);
        cmd.setFrontFace(vk::FrontFace::eClockwise);
        cmd.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);


        // Bind shared vertex and index buffers
        vk::Buffer vertexBuffers[] = {*_vertexBuffer.data};
        vk::DeviceSize offsets[] = {0};
        cmd.bindVertexBuffers(0, vertexBuffers, offsets);
        cmd.bindIndexBuffer(*_indexBuffer.data, 0, vk::IndexType::eUint16);

        // Draw Object 1
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *_pipelineLayout, 0, *descriptorSets[_gpu.getCurrentFrame()], nullptr);
        cmd.drawIndexed(std::size(Indices), 1, 0, 0, 0);

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *_pipelineLayout, 0, *descriptorSets2[_gpu.getCurrentFrame()], nullptr);
        cmd.drawIndexed(std::size(Indices), 1, 0, 0, 0);

    }

    void GUI::init() {
        createDescriptorSetLayout();
        createPipelineLayout();
        createPipeline();
        createVertexBuffer();
        createIndexBuffer();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
    }

    void GUI::createDescriptorSetLayout() {
        vk::DescriptorSetLayoutBinding vertexLayoutBinding{};
        vertexLayoutBinding.setBinding(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setPImmutableSamplers(nullptr);

        vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.setBinding(1)
                            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                            .setDescriptorCount(1)
                            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                            .setPImmutableSamplers(nullptr);

        vk::DescriptorSetLayoutBinding guistyleLayoutBinding{};
        guistyleLayoutBinding.setBinding(2)
                                  .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                  .setDescriptorCount(1)
                                  .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                                  .setPImmutableSamplers(nullptr);

        std::array bindings = { vertexLayoutBinding, samplerLayoutBinding, guistyleLayoutBinding };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.setBindingCount( bindings.size())
                  .setPBindings(bindings.data());

        _descriptorSetLayout.emplace(_gpu.getDevice(), layoutInfo);
    }

    void GUI::createPipelineLayout() {
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setSetLayoutCount(1)
        .setPSetLayouts(&**_descriptorSetLayout);

        _pipelineLayout.emplace(_gpu.getDevice(), pipelineLayoutInfo);
    }

    void GUI::createPipeline() {
        auto vertCode = core::loadShader("shaders/gui_shader.vert.spv");
        auto fragCode = core::loadShader("shaders/gui_shader.frag.spv");
        vk::raii::ShaderModule vertModule(_gpu.getDevice(), vk::ShaderModuleCreateInfo{ {}, vertCode.size(), reinterpret_cast<const uint32_t*>(vertCode.data()) });
        vk::raii::ShaderModule fragModule(_gpu.getDevice(), vk::ShaderModuleCreateInfo{ {}, fragCode.size(), reinterpret_cast<const uint32_t*>(fragCode.data()) });

        std::array stages = {
            vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex, *vertModule, "main" },
            vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, *fragModule, "main" }
        };

        vk::PipelineVertexInputStateCreateInfo vertexInput{};

        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.setBinding(0)
                          .setStride(sizeof(GUIVertex))
                          .setInputRate(vk::VertexInputRate::eVertex);

        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].setBinding(0)
                                .setLocation(0)
                                .setFormat(vk::Format::eR32G32Sfloat)
                                .setOffset(0);

        attributeDescriptions[1].setBinding(0)
                                .setLocation(1)
                                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                                .setOffset(offsetof(GUIVertex, color));

        attributeDescriptions[2].setBinding(0)
                                .setLocation(2)
                                .setFormat(vk::Format::eR32G32Sfloat)
                                .setOffset(offsetof(GUIVertex, texCoord));

        vertexInput.setVertexBindingDescriptionCount(1)
                   .setVertexAttributeDescriptionCount( attributeDescriptions.size())
                   .setPVertexBindingDescriptions(&bindingDescription)
                   .setPVertexAttributeDescriptions(attributeDescriptions.data());


        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList)
                     .setPrimitiveRestartEnable(false);

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.setViewportCount(1).setScissorCount(1);

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.setPolygonMode(vk::PolygonMode::eFill)
                  .setDepthBiasEnable(false)
                  .setDepthClampEnable(false)
                  .setRasterizerDiscardEnable(false)
                  .setLineWidth(1.0f);

        vk::PipelineMultisampleStateCreateInfo multisample{};
        multisample.setRasterizationSamples(vk::SampleCountFlagBits::e1);

        vk::PipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.setBlendEnable(true)
                       .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha) // Use alpha for color
                       .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha) // 1 - alpha for background
                       .setColorBlendOp(vk::BlendOp::eAdd) // Add blended colors
                       .setSrcAlphaBlendFactor(vk::BlendFactor::eOne) // Preserve alpha
                       .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                       .setAlphaBlendOp(vk::BlendOp::eAdd)
                       .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo blendState{};
        blendState.setAttachmentCount(1).setPAttachments(&blendAttachment);

        std::array dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eCullMode,
                                     vk::DynamicState::eFrontFace, vk::DynamicState::ePrimitiveTopology };
        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.setDynamicStateCount(dynamicStates.size()).setPDynamicStates(dynamicStates.data());



        vk::PipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.setColorAttachmentCount(1)
                     .setPColorAttachmentFormats(&_gpu.getSwapchainFormat())
                     .setDepthAttachmentFormat(_gpu.getDepthFormat());

        vk::PipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.setDepthTestEnable(true)
               .setDepthWriteEnable(true)
               .setDepthCompareOp(vk::CompareOp::eLess)
               .setDepthBoundsTestEnable(false)
               .setStencilTestEnable(false);

        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.setStageCount(stages.size())
                    .setPStages(stages.data())
                    .setPVertexInputState(&vertexInput)
                    .setPInputAssemblyState(&inputAssembly)
                    .setPViewportState(&viewportState)
                    .setPRasterizationState(&rasterizer)
                    .setPMultisampleState(&multisample)
                    .setPColorBlendState(&blendState)
                    .setPDynamicState(&dynamicState)
                    .setPDepthStencilState(&depthStencil)
                    .setLayout(**_pipelineLayout)
                    .setRenderPass(nullptr)
                    .setSubpass(0)
                    .setPNext(&renderingInfo);


        _graphicsPipeline.emplace(_gpu.getDevice(), nullptr, pipelineInfo);
    }

    void GUI::createVertexBuffer() {
        gpu::vulkan::Buffer stagingBuffer{};
        _gpu.createBuffer(GUI_GEOMETRY_BUFFER_SIZE, vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          stagingBuffer);

        // copy the vertex and color data into that device memory
        auto pData = static_cast<uint8_t *>( stagingBuffer.memory->mapMemory( 0, GUI_GEOMETRY_BUFFER_SIZE ) );
        memcpy( pData, Geometry, GUI_GEOMETRY_BUFFER_SIZE );
        stagingBuffer.memory->unmapMemory();

        _gpu.createBuffer(GUI_GEOMETRY_BUFFER_SIZE, vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eVertexBuffer,
                          vk::MemoryPropertyFlagBits::eDeviceLocal, _vertexBuffer);
        _gpu.copyBuffer(stagingBuffer, _vertexBuffer, GUI_GEOMETRY_BUFFER_SIZE);
    }

    void GUI::createIndexBuffer() {
        gpu::vulkan::Buffer stagingBuffer{};
        _gpu.createBuffer(GUI_INDEX_BUFFER_SIZE, vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          stagingBuffer);
        // copy the vertex and color data into that device memory
        auto pData = static_cast<uint8_t *>( stagingBuffer.memory->mapMemory( 0, GUI_INDEX_BUFFER_SIZE ) );
        memcpy( pData, Indices, GUI_INDEX_BUFFER_SIZE );
        stagingBuffer.memory->unmapMemory();

        _gpu.createBuffer(GUI_INDEX_BUFFER_SIZE, vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eIndexBuffer,
                          vk::MemoryPropertyFlagBits::eDeviceLocal, _indexBuffer);
        _gpu.copyBuffer(stagingBuffer, _indexBuffer, GUI_INDEX_BUFFER_SIZE);
    }

    void GUI::createTextureImage() {
        const std::string filename = "Contents/statue-1275469_1280.jpg";
        std::string path = SDL_GetBasePath() + filename;

        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> rawSurface
        {IMG_Load(path.c_str()), SDL_DestroySurface};
        if (!rawSurface)
            throw std::runtime_error(std::string("Failed to load texture: ") + SDL_GetError());

        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> convSurface
        {SDL_ConvertSurface(rawSurface.get(), SDL_PIXELFORMAT_ABGR8888), SDL_DestroySurface};
        if (!convSurface)
            throw std::runtime_error(std::string("Failed to convert texture: ") + SDL_GetError());

        textureImage.format = vk::Format::eR8G8B8A8Srgb;
        textureImage.extent = vk::Extent2D{static_cast<uint32_t>(convSurface->w), static_cast<uint32_t>(convSurface->h)};
        vk::DeviceSize imageSize = textureImage.extent.width * textureImage.extent.height * 4;

        gpu::vulkan::Buffer stagingBuffer{};
        _gpu.createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer);

        auto pData = static_cast<uint8_t *>( stagingBuffer.memory->mapMemory( 0, imageSize ) );
        memcpy( pData, convSurface->pixels,  imageSize);
        stagingBuffer.memory->unmapMemory();

        _gpu.createImage(vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage);

        _gpu.transitionImageLayout(textureImage,vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        _gpu.copyBufferToImage(stagingBuffer, textureImage);
        _gpu.transitionImageLayout(textureImage,vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    void GUI::createTextureImageView() {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setImage(*textureImage.data)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(textureImage.format)
                .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        textureImage.view.emplace(_gpu.getDevice(), viewInfo);
    }

    void GUI::createTextureSampler() {
        vk::PhysicalDeviceProperties deviceProperties = _gpu.getPhysicalDevice().getProperties();

        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.setMagFilter(vk::Filter::eLinear)
                    .setMinFilter(vk::Filter::eLinear)
                    .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                    .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                    .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                    .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
                    .setAnisotropyEnable(true)
                    .setMaxAnisotropy(deviceProperties.limits.maxSamplerAnisotropy)
                    .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
                    .setUnnormalizedCoordinates(false)
                    .setCompareEnable(false)
                    .setCompareOp(vk::CompareOp::eAlways)
                    .setMinLod(0.0f)
                    .setMaxLod(0.0f)
                    .setMipLodBias(0.0f);

        textureSampler.emplace(_gpu.getDevice(), samplerInfo);
    }

    void GUI::createUniformBuffers() {
        uniformBuffers.reserve(gpu::vulkan::MAX_FRAMES_IN_FLIGHT); // Create elements
        uniformBuffersMapped.reserve(gpu::vulkan::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < gpu::vulkan::MAX_FRAMES_IN_FLIGHT; i++) {
            gpu::vulkan::Buffer buffer{};

            _gpu.createBuffer(core::TRANSFORM_BUFFER_SIZE, vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                buffer);

            if (!buffer.memory) {
                throw std::runtime_error("Buffer memory is not initialized for buffer " + std::to_string(i));
            }
            auto mapped = static_cast<uint8_t *>(buffer.memory->mapMemory(0, core::TRANSFORM_BUFFER_SIZE));
            uniformBuffers.emplace_back(std::move(buffer));
            uniformBuffersMapped.emplace_back(mapped);
        }

        uniformBuffers2.reserve(gpu::vulkan::MAX_FRAMES_IN_FLIGHT); // Create elements
        uniformBuffersMapped2.reserve(gpu::vulkan::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < gpu::vulkan::MAX_FRAMES_IN_FLIGHT; i++) {
            gpu::vulkan::Buffer buffer{};

            _gpu.createBuffer(core::TRANSFORM_BUFFER_SIZE, vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                buffer);

            if (!buffer.memory) {
                throw std::runtime_error("Buffer memory is not initialized for buffer " + std::to_string(i));
            }
            auto mapped = static_cast<uint8_t *>(buffer.memory->mapMemory(0, core::TRANSFORM_BUFFER_SIZE));
            uniformBuffers2.emplace_back(std::move(buffer));
            uniformBuffersMapped2.emplace_back(mapped);
        }

    }

    void GUI::createDescriptorPool() {
        std::array<vk::DescriptorPoolSize, 3> poolSize{};
        poolSize[0].setType(vk::DescriptorType::eUniformBuffer)
                   .setDescriptorCount(gpu::vulkan::MAX_FRAMES_IN_FLIGHT*2);
        poolSize[1].setType(vk::DescriptorType::eCombinedImageSampler)
                   .setDescriptorCount(gpu::vulkan::MAX_FRAMES_IN_FLIGHT*2);
        poolSize[2].setType(vk::DescriptorType::eUniformBuffer)
                   .setDescriptorCount(gpu::vulkan::MAX_FRAMES_IN_FLIGHT*2);

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.setPoolSizeCount(poolSize.size())
                .setPPoolSizes(poolSize.data())
                .setMaxSets(gpu::vulkan::MAX_FRAMES_IN_FLIGHT*2)
                .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

        _descriptorPool.emplace(_gpu.getDevice(), poolInfo);
    }

    void GUI::createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(gpu::vulkan::MAX_FRAMES_IN_FLIGHT*2, *_descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(*_descriptorPool)
                 .setDescriptorSetCount(gpu::vulkan::MAX_FRAMES_IN_FLIGHT *2)
                 .setPSetLayouts(layouts.data());

        descriptorSets.reserve(gpu::vulkan::MAX_FRAMES_IN_FLIGHT);
        descriptorSets2.reserve(gpu::vulkan::MAX_FRAMES_IN_FLIGHT);
        auto sets = _gpu.getDevice().allocateDescriptorSets(allocInfo);


        for (size_t i = 0; i < gpu::vulkan::MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo bufferInfo{};
            bufferInfo.setBuffer(*uniformBuffers[i].data)
                      .setOffset(0)
                      .setRange(core::TRANSFORM_BUFFER_SIZE);

            vk::DescriptorImageInfo imageInfo{};
            imageInfo.setImageView(*textureImage.view)
                     .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                     .setSampler(*textureSampler);


            vk::DescriptorBufferInfo roundCornerInfo{};
            const GUIStyleBuffer& style = _styleContainer[0];
            roundCornerInfo.setBuffer(*style.buffer[i].data)
                           .setOffset(0)
                           .setRange(GUI_STYLE_BUFFER_SIZE);


            std::array<vk::WriteDescriptorSet,3> write{};
            write[0].setDstSet(*sets[i])
                    .setDstBinding(0)
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                    .setDescriptorCount(1)
                    .setPBufferInfo(&bufferInfo);
            write[1].setDstSet(*sets[i])
                    .setDstBinding(1)
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                    .setDescriptorCount(1)
                    .setPImageInfo(&imageInfo);
            write[2].setDstSet(*sets[i])
                    .setDstBinding(2)
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                    .setDescriptorCount(1)
                    .setPBufferInfo(&roundCornerInfo);

            // Object 2
            vk::DescriptorBufferInfo bufferInfo2{};
            bufferInfo2.setBuffer(*uniformBuffers2[i].data)
                       .setOffset(0)
                       .setRange(core::TRANSFORM_BUFFER_SIZE);



            std::array<vk::WriteDescriptorSet, 3> write2{};
            write2[0].setDstSet(*sets[i + gpu::vulkan::MAX_FRAMES_IN_FLIGHT])
                     .setDstBinding(0)
                     .setDstArrayElement(0)
                     .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                     .setDescriptorCount(1)
                     .setPBufferInfo(&bufferInfo2);
            write2[1].setDstSet(*sets[i + gpu::vulkan::MAX_FRAMES_IN_FLIGHT])
                     .setDstBinding(1)
                     .setDstArrayElement(0)
                     .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                     .setDescriptorCount(1)
                     .setPImageInfo(&imageInfo);
            write2[2].setDstSet(*sets[i + gpu::vulkan::MAX_FRAMES_IN_FLIGHT])
                     .setDstBinding(2)
                     .setDstArrayElement(0)
                     .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                     .setDescriptorCount(1)
                     .setPBufferInfo(&roundCornerInfo);

            descriptorSets.emplace_back(std::move(sets[i]));
            descriptorSets2.emplace_back(std::move(sets[i + gpu::vulkan::MAX_FRAMES_IN_FLIGHT]));

            std::vector<vk::WriteDescriptorSet> writes;
            writes.insert(writes.end(), write.begin(), write.end());
            writes.insert(writes.end(), write2.begin(), write2.end());
            _gpu.getDevice().updateDescriptorSets(writes, nullptr);
        }
    }
} // namespace ufox::ui