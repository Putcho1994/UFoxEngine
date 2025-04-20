//
// Created by b-boy on 19.04.2025.
//

#include "ufox_graphic.hpp"

namespace ufox::graphics::vulkan {
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

        createSwapChain(window);
    }

    void GraphicsDevice::createSwapChain(const windowing::sdl::UfoxWindow& window) {
        vk::SurfaceCapabilitiesKHR capabilities = physicalDevice->getSurfaceCapabilitiesKHR(*surface);
        auto formats = physicalDevice->getSurfaceFormatsKHR(*surface);
        auto presentModes = physicalDevice->getSurfacePresentModesKHR(*surface);

        vk::SurfaceFormatKHR surfaceFormat;
        for (const auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                surfaceFormat = format;
        }
        swapchainFormat = surfaceFormat.format;

        auto mailboxIt = std::ranges::find_if(presentModes,
    [](const vk::PresentModeKHR& mode) { return mode == vk::PresentModeKHR::eMailbox; });
        presentMode = mailboxIt != presentModes.end() ? *mailboxIt : useVsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate;

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

        uint32_t imageCount = capabilities.minImageCount == 1 ? 2 : capabilities.minImageCount;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.setSurface(*surface)
            .setMinImageCount(imageCount)
            .setImageFormat(swapchainFormat)
            .setImageColorSpace(surfaceFormat.colorSpace) // Use colorSpace from surfaceFormat
            .setImageExtent(swapchainExtent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setPreTransform(capabilities.currentTransform)
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
        swapchainImages = swapchain->getImages();
    }
}

