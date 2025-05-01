//
// Created by b-boy on 19.04.2025.
//

#include "ufox_graphic.hpp"

#include <barrier>


namespace ufox::graphics::vulkan {
    uint32_t FindMemoryType(const vk::PhysicalDeviceMemoryProperties &memoryProperties, uint32_t typeBits,
                            vk::MemoryPropertyFlags requirementsMask){
        auto typeIndex = static_cast<uint32_t>(~0);
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

   void TransitionImageLayout(const vk::raii::CommandBuffer& cmd, const vk::Image& image, const vk::Format& format,
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
            .setSubresourceRange({ vk::ImageAspectFlagBits::eNone, 0, 1, 0, 1 });

        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {

            if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint) {
                barrier.subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
            }
            else
                barrier.subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eDepth);
        } else {
            barrier.subresourceRange.setAspectMask( vk::ImageAspectFlagBits::eColor);
        }


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
            .setApiVersion(VK_API_VERSION_1_4);

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

            vk::PhysicalDeviceFeatures2 features2{};
            features2.features.setSamplerAnisotropy(true);
            features2.setPNext(&vulkan13Features);


            vk::DeviceCreateInfo createDeviceInfo{};
            createDeviceInfo.setQueueCreateInfoCount(static_cast<uint32_t>(queueInfos.size()))
                .setPQueueCreateInfos(queueInfos.data())
                .setEnabledExtensionCount(static_cast<uint32_t>(requiredDeviceExtensions.size()))
                .setPpEnabledExtensionNames(requiredDeviceExtensions.data())
                .setPNext(&features2);

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
        createDepthImage();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
    }

    void GraphicsDevice::waitForIdle() const {
        if (!device) return;
        device->waitIdle();
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

    void GraphicsDevice::createDepthImage() {
        // Find a supported depth format
        std::vector<vk::Format> candidates = {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint
        };
        depthImage.format = findSupportedFormat(
            candidates,
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );

        // Set extent to match swapchain
        depthImage.extent = swapchainExtent;

        // Create depth image
        createImage(
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            depthImage
        );

        // Create image view
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setImage(*depthImage.data)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(depthImage.format)
                .setSubresourceRange({
                    depthImage.format == vk::Format::eD32Sfloat ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                    0, 1, 0, 1
                });
        depthImage.view.emplace(*device, viewInfo);

        // Transition to depth-stencil attachment layout
        transitionImageLayout(
            depthImage,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        );
    }


    void GraphicsDevice::createDescriptorSetLayout() {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.setBinding(0)
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

        std::array bindings = { uboLayoutBinding, samplerLayoutBinding };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.setBindingCount( bindings.size())
                  .setPBindings(bindings.data());

        descriptorSetLayout.emplace(*device, layoutInfo);
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
                          .setStride(sizeof(Vertex))
                          .setInputRate(vk::VertexInputRate::eVertex);

        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].setBinding(0)
                                .setLocation(0)
                                .setFormat(vk::Format::eR32G32B32Sfloat)
                                .setOffset(0);

        attributeDescriptions[1].setBinding(0)
                                .setLocation(1)
                                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                                .setOffset(offsetof(Vertex, color));

        attributeDescriptions[2].setBinding(0)
                                .setLocation(2)
                                .setFormat(vk::Format::eR32G32Sfloat)
                                .setOffset(offsetof(Vertex, texCoord));

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

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setSetLayoutCount(1)
        .setPSetLayouts(&**descriptorSetLayout);

        pipelineLayout.emplace(*device, pipelineLayoutInfo);

        vk::PipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.setColorAttachmentCount(1)
                     .setPColorAttachmentFormats(&swapchainFormat)
                     .setDepthAttachmentFormat(depthImage.format);

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
                    .setLayout(**pipelineLayout)
                    .setRenderPass(nullptr)
                    .setSubpass(0)
                    .setPNext(&renderingInfo);


        graphicsPipeline.emplace(*device, nullptr, pipelineInfo);
    }

    void GraphicsDevice::createTextureImage() {
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

        Buffer stagingBuffer{};
        createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer);

        auto pData = static_cast<uint8_t *>( stagingBuffer.memory->mapMemory( 0, imageSize ) );
        memcpy( pData, convSurface->pixels,  imageSize);
        stagingBuffer.memory->unmapMemory();

        createImage(vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage);

        transitionImageLayout(textureImage,vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage(stagingBuffer, textureImage);
        transitionImageLayout(textureImage,vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    void GraphicsDevice::createTextureImageView() {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setImage(*textureImage.data)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(textureImage.format)
                .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        textureImage.view.emplace(*device, viewInfo);
    }

    void GraphicsDevice::createTextureSampler() {

        vk::PhysicalDeviceProperties deviceProperties = physicalDevice->getProperties();

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

        textureSampler.emplace(*device, samplerInfo);
    }

    void GraphicsDevice::createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(TestRect);

        Buffer stagingBuffer{};
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer);

        // copy the vertex and color data into that device memory
        auto pData = static_cast<uint8_t *>( stagingBuffer.memory->mapMemory( 0, bufferSize ) );
        memcpy( pData, TestRect, bufferSize );
        stagingBuffer.memory->unmapMemory();

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vertexBuffer);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    }

    void GraphicsDevice::createIndexBuffer() {
        vk::DeviceSize bufferSize = sizeof(indices);

        Buffer stagingBuffer{};
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer);

        // copy the vertex and color data into that device memory
        auto pData = static_cast<uint8_t *>( stagingBuffer.memory->mapMemory( 0, bufferSize ) );
        memcpy( pData, indices, bufferSize );
        stagingBuffer.memory->unmapMemory();

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            indexBuffer);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    }

    void GraphicsDevice::createUniformBuffers() {
        const vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT); // Create elements
        uniformBuffersMapped.reserve(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            Buffer buffer{};

            createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                buffer);

            if (!buffer.memory) {
                throw std::runtime_error("Buffer memory is not initialized for buffer " + std::to_string(i));
            }
            auto mapped = static_cast<uint8_t *>(buffer.memory->mapMemory(0, bufferSize));
            uniformBuffers.emplace_back(std::move(buffer));
            uniformBuffersMapped.emplace_back(mapped);
        }
    }

    void GraphicsDevice::updateUniformBuffer(uint32_t currentImage) const {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height), 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void GraphicsDevice::createDescriptorPool() {
        std::array<vk::DescriptorPoolSize, 2> poolSize{};
        poolSize[0].setType(vk::DescriptorType::eUniformBuffer)
                   .setDescriptorCount(MAX_FRAMES_IN_FLIGHT);
        poolSize[1].setType(vk::DescriptorType::eCombinedImageSampler)
                   .setDescriptorCount(MAX_FRAMES_IN_FLIGHT);

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.setPoolSizeCount(poolSize.size())
                .setPPoolSizes(poolSize.data())
                .setMaxSets(MAX_FRAMES_IN_FLIGHT)
                .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

        descriptorPool.emplace(*device, poolInfo);
    }

    void GraphicsDevice::createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(*descriptorPool)
                 .setDescriptorSetCount(MAX_FRAMES_IN_FLIGHT)
                 .setPSetLayouts(layouts.data());

        descriptorSets.reserve(MAX_FRAMES_IN_FLIGHT);
        descriptorSets = device->allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo bufferInfo{};
            bufferInfo.setBuffer(*uniformBuffers[i].data)
                      .setOffset(0)
                      .setRange(sizeof(UniformBufferObject));

            vk::DescriptorImageInfo imageInfo{};
            imageInfo.setImageView(*textureImage.view)
                     .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                     .setSampler(*textureSampler);


            std::array<vk::WriteDescriptorSet,2> write{};
            write[0].setDstSet(*descriptorSets[i])
                 .setDstBinding(0)
                 .setDstArrayElement(0)
                 .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                 .setDescriptorCount(1)
                 .setPBufferInfo(&bufferInfo);
            write[1].setDstSet(*descriptorSets[i])
                 .setDstBinding(1)
                 .setDstArrayElement(0)
                 .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                 .setDescriptorCount(1)
                 .setPImageInfo(&imageInfo);

            device->updateDescriptorSets(write, nullptr);
        }
    }

    void GraphicsDevice::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                      vk::MemoryPropertyFlags properties, Buffer &buffer) {

        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.setSize(size)
                  .setUsage(usage)
                  .setSharingMode(vk::SharingMode::eExclusive);

        buffer.data.emplace(*device, bufferInfo);

        vk::MemoryRequirements memoryRequirements = buffer.data->getMemoryRequirements();
        vk::MemoryAllocateInfo memoryAllocateInfo( memoryRequirements.size, FindMemoryType( physicalDevice->getMemoryProperties(),
                                                   memoryRequirements.memoryTypeBits, properties ) );
        buffer.memory.emplace(*device, memoryAllocateInfo);
        buffer.data->bindMemory( *buffer.memory, 0 );
    }

    void GraphicsDevice::createImage(vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, Image& image) {

        vk::ImageCreateInfo imageInfo{};
        imageInfo.setImageType(vk::ImageType::e2D)
                 .setFormat(image.format)
                 .setExtent({image.extent.width, image.extent.height, 1})
                 .setMipLevels(1)
                 .setArrayLayers(1)
                 .setSamples(vk::SampleCountFlagBits::e1)
                 .setTiling(tiling)
                 .setInitialLayout(vk::ImageLayout::eUndefined)
                 .setUsage(usage)
                 .setSamples(vk::SampleCountFlagBits::e1)
                 .setSharingMode(vk::SharingMode::eExclusive);

        image.data.emplace(*device, imageInfo);

        vk::MemoryRequirements memoryRequirements = image.data->getMemoryRequirements();
        vk::MemoryAllocateInfo memoryAllocateInfo( memoryRequirements.size, FindMemoryType( physicalDevice->getMemoryProperties(),
                                                   memoryRequirements.memoryTypeBits, properties ) );
        image.memory.emplace(*device, memoryAllocateInfo);
        image.data->bindMemory( *image.memory, 0 );
    }

    void GraphicsDevice::copyBuffer(const Buffer& srcBuffer, const Buffer& dstBuffer, const vk::DeviceSize& size) const {
        vk::raii::CommandBuffer cmd = beginSingleTimeCommands();

        vk::BufferCopy copyRegion{};
        copyRegion.setSize(size);
        cmd.copyBuffer(*srcBuffer.data, *dstBuffer.data, { copyRegion });

        endSingleTimeCommands(cmd);
    }

    vk::raii::CommandBuffer GraphicsDevice::beginSingleTimeCommands() const {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setLevel(vk::CommandBufferLevel::ePrimary)
                 .setCommandPool(*commandPool)
                 .setCommandBufferCount(1);

        vk::raii::CommandBuffer cmd = std::move(device->allocateCommandBuffers(allocInfo).front());
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        cmd.begin(beginInfo);

        return cmd;
    }

    void GraphicsDevice::endSingleTimeCommands(const vk::raii::CommandBuffer &cmd) const {
        cmd.end();
        vk::SubmitInfo submitInfo{};
        submitInfo.setCommandBufferCount(1)
                 .setPCommandBuffers(&*cmd);
        graphicsQueue->submit(submitInfo, nullptr);
        graphicsQueue->waitIdle();
    }

    void GraphicsDevice::transitionImageLayout(const Image& image, vk::ImageLayout oldLayout,vk::ImageLayout newLayout) const {
        vk::raii::CommandBuffer cmd = beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier{};
    barrier.setOldLayout(oldLayout)
           .setNewLayout(newLayout)
           .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
           .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
           .setImage(*image.data)
           .setSubresourceRange({
               image.format == vk::Format::eR8G8B8A8Srgb ? vk::ImageAspectFlagBits::eColor :
               (image.format == vk::Format::eD32Sfloat ? vk::ImageAspectFlagBits::eDepth :
                vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil),
               0, 1, 0, 1
           });

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.setSrcAccessMask({})
               .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
               .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.setSrcAccessMask({})
               .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    cmd.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, { barrier });

    endSingleTimeCommands(cmd);
    }

    void GraphicsDevice::copyBufferToImage(const Buffer& buffer, const Image& image) const {
        vk::raii::CommandBuffer cmd = beginSingleTimeCommands();

        vk::BufferImageCopy region{};
        region.setImageSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 })
              .setImageOffset({ 0, 0, 0 })
              .setImageExtent({image.extent.width, image.extent.height, 1})
              .setBufferOffset(0)
              .setBufferRowLength(0)
              .setBufferImageHeight(0);

        cmd.copyBufferToImage(*buffer.data, *image.data, vk::ImageLayout::eTransferDstOptimal, { region });

        endSingleTimeCommands(cmd);
    }

    vk::Format GraphicsDevice::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
        vk::FormatFeatureFlags features) const {

        for(vk::Format format : candidates) {
            vk::FormatProperties props = physicalDevice->getFormatProperties(format);
            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    void GraphicsDevice::recreateSwapchain(const windowing::sdl::UfoxWindow& window) {
        waitForIdle();
        depthImage.clear();
        swapchainImageViews.clear();
        swapchain.reset();
        createSwapchain(window);
        createDepthImage();
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

        TransitionImageLayout(cmd, swapchainImages[imageIndex], swapchainFormat,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput);


        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.setImageView(*swapchainImageViews[imageIndex])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue({ std::array{0.2f, 0.2f, 0.2f, 1.0f} });

        vk::RenderingAttachmentInfo depthAttachment{};
        depthAttachment.setImageView(*depthImage.view)
                       .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                       .setLoadOp(vk::AttachmentLoadOp::eClear)
                       .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                       .setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0)));


        vk::RenderingInfo renderingInfo{};
        renderingInfo.setRenderArea({ {0, 0}, swapchainExtent })
            .setLayerCount(1)
            .setColorAttachmentCount(1)
            .setPColorAttachments(&colorAttachment)
            .setPDepthAttachment(&depthAttachment);

        cmd.beginRendering(renderingInfo);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

        cmd.setViewport(0, vk::Viewport{ 0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f });
        cmd.setScissor(0, vk::Rect2D{ {0, 0}, swapchainExtent });
        cmd.setCullMode(vk::CullModeFlagBits::eNone);
        cmd.setFrontFace(vk::FrontFace::eClockwise);
        cmd.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);

        // cmd.clearColorImage(swapchainImages[imageIndex], vk::ImageLayout::eUndefined, vk::ClearColorValue{0.023f,0.033f,0.033f,1.0f},
        //             vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0,1,0,1});

        vk::Buffer vertexBuffers[] = {*vertexBuffer.data};
        vk::DeviceSize offsets[] = {0};

        cmd.bindVertexBuffers( 0, vertexBuffers, offsets );
        cmd.bindIndexBuffer( *indexBuffer.data, 0, vk::IndexType::eUint16 );
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descriptorSets[currentFrame], nullptr);

        cmd.drawIndexed(static_cast<uint32_t>(std::size(indices)), 1, 0, 0, 0);

        cmd.endRendering();

        TransitionImageLayout(cmd, swapchainImages[imageIndex], swapchainFormat,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe);



        cmd.end();

        updateUniformBuffer(currentFrame);

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
}

