//
// Created by b-boy on 19.04.2025.
//

#include "ufox_graphic.hpp"

namespace ufox::graphics::vulkan {

    uint32_t FindMemoryType(vk::PhysicalDeviceMemoryProperties const &memoryProperties, uint32_t typeBits,
        vk::MemoryPropertyFlags requirementsMask){
        uint32_t typeIndex = static_cast<uint32_t>(~0);
        for ( uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++ )
        {
            if ( typeBits & 1 && ( memoryProperties.memoryTypes[i].propertyFlags & requirementsMask ) == requirementsMask )
            {
                typeIndex = i;
                break;
            }
            typeBits >>= 1;
        }
        assert( typeIndex != static_cast<uint32_t>(~0));
        return typeIndex;
    }

    void TransitionImageLayout(const vk::raii::CommandBuffer& cmd, const vk::Image& image,
                               vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                               vk::AccessFlags2 srcAccess, vk::AccessFlags2 dstAccess,
                               vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage){
        vk::ImageMemoryBarrier2 barrier{};
        barrier.setImage(image)
            .setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setSrcStageMask(srcStage)
            .setDstStageMask(dstStage)
            .setSrcAccessMask(srcAccess)
            .setDstAccessMask(dstAccess)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        vk::DependencyInfo dependency{};
        dependency.setImageMemoryBarrierCount(1)
            .setPImageMemoryBarriers(&barrier);
        cmd.pipelineBarrier2(dependency);
    }

    std::vector<char> loadShader(const std::string& filename){
        std::string path = SDL_GetBasePath() + filename;
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open shader: " + path);

        size_t size = file.tellg();
        if (size > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
            throw std::runtime_error("File size exceeds maximum streamsize limit");
        }

        std::vector<char> buffer(size);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(size));
        file.close();
        return buffer;
    }

    bool AreExtensionsSupported( const std::vector<const char *> &required, const std::vector<vk::ExtensionProperties> &available) {
        for (const auto* req : required) {
            bool found = false;
            for (const auto& avail : available) {
                if (strcmp(req, avail.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                fmt::println("Missing extension: {}", req);
                return false;
            }
        }
        return true;
    }

    GraphicsDevice::GraphicsDevice(const windowing::sdl::UfoxWindow& window, const char* engineName, uint32_t engineVersion, const char* appName, uint32_t appVersion) {
#pragma region Create Context
        auto vkGetInstanceProcAddr{reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr())};
        context.emplace(vkGetInstanceProcAddr);
        auto const vulkanVersion {context->enumerateInstanceVersion()};
        fmt::println("Vulkan API version: {}.{}", VK_VERSION_MAJOR(vulkanVersion), VK_VERSION_MINOR(vulkanVersion));
#pragma endregion

#pragma region Create Instance
        vk::ApplicationInfo appInfo{};
        appInfo.setPApplicationName(appName)
            .setApplicationVersion(appVersion)
            .setPEngineName(engineName)
            .setEngineVersion(engineVersion)
            .setApiVersion(vulkanVersion);

        std::vector<vk::ExtensionProperties> availableInstanceExtensions = context->enumerateInstanceExtensionProperties();
        std::vector<const char*> requiredInstanceExtensions{};

        uint32_t extensionCount = 0;
        const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
        if (extensions == nullptr) throw std::runtime_error("Failed to get SDL Vulkan extensions");

        requiredInstanceExtensions.insert(requiredInstanceExtensions.end(), extensions, extensions + extensionCount);

#if !defined(NDEBUG)
        for (size_t i = 0; i < extensionCount; ++i) {
            fmt::println("SDL extension: {}", extensions[i]);
        }
#endif

        if (!AreExtensionsSupported(requiredInstanceExtensions, availableInstanceExtensions)) {
            throw std::runtime_error("Required device extensions are missing");
        }

        vk::InstanceCreateInfo createInfo{};
        createInfo.setPApplicationInfo(&appInfo)
            .setEnabledExtensionCount(static_cast<uint32_t>(requiredInstanceExtensions.size()))
            .setPpEnabledExtensionNames(requiredInstanceExtensions.data());

        instance.emplace(*context, createInfo);
#pragma endregion

#pragma region Create Surface
        VkSurfaceKHR raw_surface;

        if (!SDL_Vulkan_CreateSurface(window.get(),**instance, nullptr, &raw_surface))
            throw windowing::sdl::SDLException("Failed to create surface");

        surface.emplace(*instance,raw_surface);
#pragma endregion

#pragma region Select Physical Device
        bool foundBoth = false;
        auto devices = instance->enumeratePhysicalDevices();
        for (const auto& device : devices) {
            QueueFamilyIndices indices;
            auto families = device.getQueueFamilyProperties();
            for (uint32_t i = 0; i < families.size(); ++i) {
                if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) indices.graphics = i;
                if (device.getSurfaceSupportKHR(i, *surface)) indices.present = i;
                foundBoth = indices.graphics && indices.present;
                if (foundBoth) break;
            }

            if (foundBoth) { physicalDevice = device; queueFamilyIndices = indices; break; }
        }
        if (!physicalDevice) throw std::runtime_error("No suitable physical device found");
#pragma endregion

#pragma region Create Logical Device
        std::vector<vk::ExtensionProperties> availableDeviceExtensions = physicalDevice->enumerateDeviceExtensionProperties();
        std::vector requiredDeviceExtensions{ vk::KHRSwapchainExtensionName, vk::KHRSynchronization2ExtensionName };

        if (!AreExtensionsSupported(requiredDeviceExtensions, availableDeviceExtensions))
            throw std::runtime_error("Required device extensions are missing");

        std::set uniqueFamilies = { *queueFamilyIndices.graphics, *queueFamilyIndices.present };
        std::vector<vk::DeviceQueueCreateInfo> queueInfos;
        float priority = 0.0f;
        for (uint32_t family : uniqueFamilies) {
            vk::DeviceQueueCreateInfo queueInfo{};
            queueInfo.setQueueFamilyIndex(family)
                .setQueueCount(1)
                .setPQueuePriorities(&priority);
            queueInfos.push_back(queueInfo);

            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures{};
            dynamicStateFeatures.setExtendedDynamicState(true);

            vk::PhysicalDeviceVulkan13Features vulkan13Features{};
            vulkan13Features.setPNext(&dynamicStateFeatures)
                .setSynchronization2(true)
                .setDynamicRendering(true);

            vk::PhysicalDeviceFeatures2 features{};
            features.setPNext(&vulkan13Features);

            vk::DeviceCreateInfo createDeviceInfo{};
            createDeviceInfo.setQueueCreateInfoCount(static_cast<uint32_t>(queueInfos.size()))
                .setPQueueCreateInfos(queueInfos.data())
                .setEnabledExtensionCount(static_cast<uint32_t>(requiredDeviceExtensions.size()))
                .setPpEnabledExtensionNames(requiredDeviceExtensions.data())
                .setPNext(&features);

            device.emplace(*physicalDevice, createDeviceInfo);
            graphicsQueue.emplace(*device, *queueFamilyIndices.graphics, 0);
            presentQueue.emplace(*device, *queueFamilyIndices.present, 0);
        }

#pragma endregion

#pragma region Create Command Pool
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.setQueueFamilyIndex(*queueFamilyIndices.graphics)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPool.emplace(*device, poolInfo);
#pragma endregion

#pragma region Create Command Buffers
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setCommandPool(*commandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(MAX_FRAMES_IN_FLIGHT);
        commandBuffers = device->allocateCommandBuffers(allocInfo);
#pragma endregion

#pragma region Create Synchronization Objects
        vk::SemaphoreCreateInfo semaphoreInfo{};
        vk::FenceCreateInfo fenceInfo{ vk::FenceCreateFlagBits::eSignaled };

        imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            imageAvailableSemaphores.emplace_back(*device, semaphoreInfo);
            renderFinishedSemaphores.emplace_back(*device, semaphoreInfo);
            inFlightFences.emplace_back(*device, fenceInfo);
        }
#pragma endregion

        createSwapchain(window);
        createGraphicsPipeline();
        createVertexBuffer();
    }

    void GraphicsDevice::createSwapchain(const windowing::sdl::UfoxWindow& window) {
        vk::SurfaceCapabilitiesKHR capabilities = physicalDevice->getSurfaceCapabilitiesKHR(*surface);

#pragma region Get Supported Format
        auto formats = physicalDevice->getSurfaceFormatsKHR(*surface);
        vk::SurfaceFormatKHR surfaceFormat;
        for (const auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                surfaceFormat = format;
        }
        swapchainFormat = surfaceFormat.format;
#pragma endregion

#pragma region Get Supported Present Mode
        auto presentModes = physicalDevice->getSurfacePresentModesKHR(*surface);
        auto mailboxIt = std::ranges::find_if(presentModes,
    [](const vk::PresentModeKHR& mode) { return mode == vk::PresentModeKHR::eMailbox; });
        presentMode = mailboxIt != presentModes.end() ? *mailboxIt : useVsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate;
#pragma endregion

#pragma region Get Extent
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            swapchainExtent = capabilities.currentExtent;
        }
        else {
            auto [windowExtentW, windowExtentH] = window.getSize();
            vk::Extent2D extent { windowExtentW, windowExtentH };
            extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            swapchainExtent = extent;
        }
#pragma endregion

#pragma region Get Count
        uint32_t imageCount = capabilities.minImageCount < 2 ? 2 : capabilities.minImageCount;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }
#pragma endregion

#pragma region Get Pre Transform support flag
        vk::SurfaceTransformFlagBitsKHR preTransform = capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity
                                                                   ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                                                                   : capabilities.currentTransform;
#pragma endregion

#pragma region Create Swapchain
        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.setSurface(*surface)
            .setMinImageCount(imageCount)
            .setImageFormat(swapchainFormat)
            .setImageColorSpace(surfaceFormat.colorSpace) // Use colorSpace from surfaceFormat
            .setImageExtent(swapchainExtent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setPreTransform(preTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(presentMode)
            .setClipped(true);

        std::array queueIndices = { *queueFamilyIndices.graphics, *queueFamilyIndices.present };
        if (queueFamilyIndices.graphics != queueFamilyIndices.present) {
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(2)
                .setPQueueFamilyIndices(queueIndices.data());
        }
        else {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        }

        swapchain.emplace(*device, createInfo);
#pragma endregion

#pragma region Create Image View
        swapchainImages = swapchain->getImages();
        swapchainImageViews.reserve(swapchainImages.size());

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setViewType(vk::ImageViewType::e2D)
            .setFormat(swapchainFormat)
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        for (const auto& image : swapchainImages) {
            viewInfo.setImage(image);
            swapchainImageViews.emplace_back(*device, viewInfo);
        }
#pragma endregion
    }

    void GraphicsDevice::createGraphicsPipeline() {
        auto vertCode = loadShader("shaders/shader.vert.spv");
        auto fragCode = loadShader("shaders/shader.frag.spv");
        vk::raii::ShaderModule vertModule(*device, vk::ShaderModuleCreateInfo{ {}, vertCode.size(), reinterpret_cast<const uint32_t*>(vertCode.data()) });
        vk::raii::ShaderModule fragModule(*device, vk::ShaderModuleCreateInfo{ {}, fragCode.size(), reinterpret_cast<const uint32_t*>(fragCode.data()) });

        std::array stages = {
            vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex, *vertModule, "main" },
            vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, *fragModule, "main" }
        };

        vk::PipelineVertexInputStateCreateInfo vertexInput{};

        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.setBinding(0)
                          .setStride(sizeof(geometry::Vertex))
                          .setInputRate(vk::VertexInputRate::eVertex);

        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].setBinding(0)
                                .setLocation(0)
                                .setFormat(vk::Format::eR32G32Sfloat)
                                .setOffset(offsetof(geometry::Vertex, pos));

        attributeDescriptions[1].setBinding(0)
                                .setLocation(1)
                                .setFormat(vk::Format::eR32G32B32Sfloat)
                                .setOffset(offsetof(geometry::Vertex, color));

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
        blendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo blendState{};
        blendState.setAttachmentCount(1).setPAttachments(&blendAttachment);

        std::array dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eCullMode,
                                     vk::DynamicState::eFrontFace, vk::DynamicState::ePrimitiveTopology };
        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.setDynamicStateCount(dynamicStates.size()).setPDynamicStates(dynamicStates.data());

        pipelineLayout.emplace(*device, vk::PipelineLayoutCreateInfo{});

        vk::PipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.setColorAttachmentCount(1).setPColorAttachmentFormats(&swapchainFormat);

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
            .setLayout(**pipelineLayout)
            .setRenderPass(nullptr)
            .setSubpass(0)
            .setPNext(&renderingInfo);


        graphicsPipeline.emplace(*device, nullptr, pipelineInfo);
    }

    void GraphicsDevice::recreateSwapchain(const windowing::sdl::UfoxWindow& window) {
        waitForIdle();

        swapchainImageViews.clear();
        swapchain.reset();
        createSwapchain(window);
    }

    void GraphicsDevice::drawFrame(const windowing::sdl::UfoxWindow& window) {
        if (!enableRender) return;

        [[maybe_unused]] auto waitResult = device->waitForFences(*inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        auto [result, imageIndex] = swapchain->acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], nullptr);
        currentImage = imageIndex;

        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain(window);
            return;
        }

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire swapchain image");
        }

        device->resetFences(*inFlightFences[currentFrame]);
        vk::raii::CommandBuffer& cmd = commandBuffers[currentFrame];
        cmd.reset();

        vk::CommandBufferBeginInfo beginInfo{};
        cmd.begin(beginInfo);

        TransitionImageLayout(cmd, swapchainImages[imageIndex],
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.setImageView(*swapchainImageViews[imageIndex])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue({ std::array{0.2f, 0.2f, 0.2f, 1.0f} });

        vk::RenderingInfo renderingInfo{};
        renderingInfo.setRenderArea({ {0, 0}, swapchainExtent })
            .setLayerCount(1)
            .setColorAttachmentCount(1)
            .setPColorAttachments(&colorAttachment);

        cmd.beginRendering(renderingInfo);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        cmd.setViewport(0, vk::Viewport{ 0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f });
        cmd.setScissor(0, vk::Rect2D{ {0, 0}, swapchainExtent });
        cmd.setCullMode(vk::CullModeFlagBits::eNone);
        cmd.setFrontFace(vk::FrontFace::eClockwise);
        cmd.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
        cmd.bindVertexBuffers( 0, { *vertexBuffer }, { 0 } );
        cmd.draw(static_cast<uint32_t>(triangle.size()), 1, 0 ,0);
        cmd.endRendering();

        TransitionImageLayout(cmd, swapchainImages[imageIndex],
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe);

        cmd.end();

        vk::SubmitInfo submitInfo{};
        vk::PipelineStageFlags waitStage{ vk::PipelineStageFlagBits::eTopOfPipe }; // Fixed from TopOfPipe
        submitInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&(*imageAvailableSemaphores[currentFrame]))
            .setPWaitDstStageMask(&waitStage)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&(*cmd))
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(&(*renderFinishedSemaphores[currentFrame]));

        graphicsQueue->submit(submitInfo, *inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&*renderFinishedSemaphores[currentFrame])
            .setSwapchainCount(1)
            .setPSwapchains(&**swapchain)
            .setPImageIndices(&imageIndex);

        vk::Result presentResult = presentQueue->presentKHR(presentInfo);
        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
            recreateSwapchain(window);
        }
        else if (presentResult != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to present swapchain image");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void GraphicsDevice::waitForIdle() const {
        if (!device) return;
        device->waitIdle();
    }

    void GraphicsDevice::createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(triangle[0]) * triangle.size();

        std::optional<vk::raii::Buffer> stagingBuffer{};
        std::optional<vk::raii::DeviceMemory> stagingBufferMemory;
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        // copy the vertex and color data into that device memory
        auto * pData = static_cast<uint8_t *>( stagingBufferMemory->mapMemory( 0, bufferSize ) );
        memcpy( pData, triangle.data(), sizeof( triangle ) );
        stagingBufferMemory->unmapMemory();

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vertexBuffer, vertexBufferMemory);

        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setLevel(vk::CommandBufferLevel::ePrimary)
                 .setCommandPool(*commandPool)
                 .setCommandBufferCount(1);

        std::vector<vk::raii::CommandBuffer> cmd = device->allocateCommandBuffers(allocInfo);
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        cmd.front().begin(beginInfo);
        vk::BufferCopy copyRegion{};
        copyRegion.setSrcOffset(0);
        copyRegion.setDstOffset(0);
        copyRegion.setSize(bufferSize);
        cmd.front().copyBuffer(*stagingBuffer, *vertexBuffer, { copyRegion });
        cmd.front().end();

        vk::SubmitInfo submitInfo{};
        submitInfo.setCommandBufferCount(1)
                 .setPCommandBuffers(&*cmd.front());

        graphicsQueue->submit(submitInfo, nullptr);
        graphicsQueue->waitIdle();
    }

    void GraphicsDevice::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties, std::optional<vk::raii::Buffer> &buffer, std::optional<vk::raii::DeviceMemory> &bufferMemory) {

        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.setSize(size)
                  .setUsage(usage)
                  .setSharingMode(vk::SharingMode::eExclusive);

        buffer.emplace(*device, bufferInfo);

        vk::MemoryRequirements memoryRequirements = buffer->getMemoryRequirements();
        uint32_t               memoryTypeIndex    = FindMemoryType( physicalDevice->getMemoryProperties(),
                                                           memoryRequirements.memoryTypeBits,
                                                           properties );
        vk::MemoryAllocateInfo memoryAllocateInfo( memoryRequirements.size, memoryTypeIndex );
        bufferMemory.emplace(*device, memoryAllocateInfo);

        buffer->bindMemory( *bufferMemory, 0 );
    }
}

namespace ufox::graphics::geometry {
    vk::VertexInputBindingDescription Vertex::getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.setBinding(0)
                          .setStride(sizeof(Vertex))
                          .setInputRate(vk::VertexInputRate::eVertex);

        return bindingDescription;
    }

    std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions()  {
        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].setBinding(0)
                                .setLocation(0)
                                .setFormat(vk::Format::eR32G32Sfloat)
                                .setOffset(offsetof(Vertex, pos));

        attributeDescriptions[1].setBinding(0)
                                .setLocation(1)
                                .setFormat(vk::Format::eR32G32B32Sfloat)
                                .setOffset(offsetof(Vertex, color));

        return attributeDescriptions;
    }
}
