//
// Created by b-boy on 19.04.2025.
//

#include "ufox_graphic.hpp"




namespace ufox::gpu::vulkan {
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

    const vk::raii::CommandBuffer* GraphicsDevice::beginFrame(const windowing::sdl::UfoxWindow &window) {

        auto waitResult = device->waitForFences(*inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        device->resetFences(*inFlightFences[currentFrame]);

        auto [result, imageIndex] = swapchain->acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], nullptr);
        currentImage = imageIndex;

        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain(window);
            return nullptr;
        }

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire swapchain image");
        }

        isFrameStarted = true;

        const vk::raii::CommandBuffer& cmd = commandBuffers[currentFrame];
        cmd.reset();
        vk::CommandBufferBeginInfo beginInfo{};
        cmd.begin(beginInfo);

        return &cmd;
    }

    void GraphicsDevice::beginDynamicRendering(const vk::raii::CommandBuffer &cmd) const{
        if (!isFrameStarted) return;

        TransitionImageLayout(cmd, swapchainImages[currentImage], swapchainFormat,
             vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
             vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
             vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput);


        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.setImageView(*swapchainImageViews[currentImage])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue({std::array{0.0f, 0.0f, 0.0f, 1.0f}});

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
    }

    void GraphicsDevice::endDynamicRendering(const vk::raii::CommandBuffer &cmd) const {
        if (!isFrameStarted) return;

        cmd.endRendering();

        TransitionImageLayout(cmd, swapchainImages[currentImage], swapchainFormat,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe);
    }

    void GraphicsDevice::endFrame(const vk::raii::CommandBuffer &cmd, const windowing::sdl::UfoxWindow &window){
        if (!isFrameStarted) return;

        cmd.end();

        vk::SubmitInfo submitInfo{};
        vk::PipelineStageFlags waitStage{ vk::PipelineStageFlagBits::eTopOfPipe }; // Fixed from TopOfPipe
        submitInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&(*imageAvailableSemaphores[currentFrame]))
            .setPWaitDstStageMask(&waitStage)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&*cmd)
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(&(*renderFinishedSemaphores[currentFrame]));

        graphicsQueue->submit(submitInfo, *inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&*renderFinishedSemaphores[currentFrame])
            .setSwapchainCount(1)
            .setPSwapchains(&**swapchain)
            .setPImageIndices(&currentImage);

        vk::Result presentResult = presentQueue->presentKHR(presentInfo);
        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
            recreateSwapchain(window);
        }
        else if (presentResult != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to present swapchain image");
        }

        isFrameStarted = false;
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void GraphicsDevice::changeBackgroundColor(const vk::raii::CommandBuffer &cmd, const glm::vec4 &color) const {

        cmd.clearColorImage(swapchainImages[currentImage], vk::ImageLayout::eUndefined, vk::ClearColorValue{color.r,color.g,color.b,color.a},
                     vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0,1,0,1});
    }

    void GraphicsDevice::recreateSwapchain(const windowing::sdl::UfoxWindow& window) {
        waitForIdle();
        depthImage.clear();
        swapchainImageViews.clear();
        swapchain.reset();
        createSwapchain(window);
        createDepthImage();
    }
}

