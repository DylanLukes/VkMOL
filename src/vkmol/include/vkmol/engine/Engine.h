#ifndef VKMOL_ENGINE_H
#define VKMOL_ENGINE_H

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#include <vkmol/Constants.h>
#include <vkmol/Debug.h>

namespace vkmol {
namespace engine {

using SurfaceFactory = typename std::function<vk::ResultValue<vk::SurfaceKHR>(
    const vk::Instance &)>;

using WindowSizeCallback = typename std::function<std::tuple<int, int>(void)>;

struct EngineCreateInfo {
  const char *AppName;
  uint32_t AppVersion;
  std::vector<const char *> InstanceExtensions;
  std::vector<const char *> DeviceExtensions;
  std::vector<const char *> ValidationLayers;

  SurfaceFactory SurfaceFactory;
  WindowSizeCallback WindowSizeCallback;
};

struct QueueFamilyIndices {
  int GraphicsFamilyIndex = -1;
  int PresentFamilyIndex = -1;

  bool isComplete() {
    return GraphicsFamilyIndex >= 0 && PresentFamilyIndex >= 0;
  }

  explicit operator std::vector<uint32_t>() {
    std::vector<uint32_t> V;
    if (GraphicsFamilyIndex > 0) V.push_back(GraphicsFamilyIndex);
    if (PresentFamilyIndex > 0) V.push_back(PresentFamilyIndex);
    return V;
  }

  explicit operator std::set<uint32_t>() {
    std::vector<uint32_t> V(*this);
    return std::set<uint32_t>(V.begin(), V.end());
  }
};

struct SwapchainSupportDetails {
  vk::SurfaceCapabilitiesKHR Capabilities;
  std::vector<vk::SurfaceFormatKHR> Formats;
  std::vector<vk::PresentModeKHR> PresentModes;
};

class Engine {
private:
  // Statics
  // -------
  static const std::vector<const char *> RequiredInstanceExtensions;
  static const std::vector<const char *> RequiredDeviceExtensions;

  static const int MaxFramesInFlight = 2;

  // Engine State
  // ------------

  vk::DebugReportCallbackCreateInfoEXT DebugReportCreateInfo = {
      vk::DebugReportFlagBitsEXT::eError |
          vk::DebugReportFlagBitsEXT::eWarning |
          vk::DebugReportFlagBitsEXT::eDebug,
      debugReportMessageEXT};

  vk::UniqueDebugReportCallbackEXT Callback;

  vk::UniqueInstance Instance;
  vk::UniqueSurfaceKHR Surface;

  vk::PhysicalDevice PhysicalDevice = nullptr;
  vk::UniqueDevice Device;

  vk::Queue GraphicsQueue;
  vk::Queue PresentQueue;

  vk::UniqueSwapchainKHR Swapchain;
  std::vector<vk::Image> SwapchainImages;
  vk::Format SwapchainImageFormat;
  vk::Extent2D SwapchainExtent;
  std::vector<vk::UniqueImageView> SwapchainImageViews;
  std::vector<vk::UniqueFramebuffer> SwapchainFramebuffers;

  vk::UniqueRenderPass RenderPass;
  vk::UniqueDescriptorSetLayout DescriptorSetLayout;
  vk::UniquePipelineLayout PipelineLayout;
  vk::UniquePipeline GraphicsPipeline;

  vk::UniqueCommandPool CommandPool;

  vk::UniqueImage DepthImage;
  vk::UniqueDeviceMemory DepthImageMemory;
  vk::UniqueImageView DepthImageView;

  vk::UniqueBuffer VertexBuffer;
  vk::UniqueDeviceMemory VertexBufferMemory;
  vk::UniqueBuffer IndexBuffer;
  vk::UniqueDeviceMemory IndexBufferMemory;

  vk::UniqueBuffer UniformBuffer;
  vk::UniqueDeviceMemory UniformBufferMemory;

  vk::UniqueDescriptorPool DescriptorPool;
  vk::UniqueDescriptorSet DescriptorSet;

  std::vector<vk::UniqueCommandBuffer> CommandBuffers;

  std::vector<vk::UniqueSemaphore> ImageAvailableSemaphores;
  std::vector<vk::UniqueSemaphore> RenderFinishedSemaphores;
  std::vector<vk::UniqueFence> InFlightFences;

  size_t CurrentFrame;

  // Initialization State
  // --------------------
  vk::ApplicationInfo ApplicationInfo;

  std::vector<const char *> InstanceExtensions;
  std::vector<const char *> DeviceExtensions;
  std::vector<const char *> ValidationLayers;

  SurfaceFactory SurfaceFactory;
  WindowSizeCallback GetWindowSize;

  // Setup Routines
  // --------------
  vk::Result createInstance();
  vk::Result createSurface();
  vk::Result installDebugCallback();
  vk::Result createPhysicalDevice();
  vk::Result createLogicalDevice();
  vk::Result createSwapchain();
  vk::Result createImageViews();
  vk::Result createRenderPass();
  vk::Result createGraphicsPipeline();
  vk::Result createFramebuffers();
  vk::Result createCommandPool();
  vk::Result createCommandBuffers();
  vk::Result createSyncObjects();

  // Setup Utilities
  // ---------------

  /**
   * Assigns a score to a PhysicalDevice. Higher is better.
   * @param Device the device to score.
   * @return a positive integer, or zero for a non-viable device.
   */
  vk::ResultValue<uint32_t> scoreDevice(const vk::PhysicalDevice &Device);

  /**
   * Checks whether a device supports the required extensions.
   * @param Device the device to check.
   * @return true if the device is viable, false otherwise.
   */
  vk::ResultValue<bool>
  checkDeviceExtensionSupport(const vk::PhysicalDevice &Device);

  /**
   * Queries the available queue families for a device.
   * @param Device the device to query queue families of.
   * @return a populated QueueFamilyIndices.
   */
  vk::ResultValue<QueueFamilyIndices>
  queryQueueFamilies(const vk::PhysicalDevice &Device);

  vk::ResultValue<SwapchainSupportDetails>
  querySwapchainSupport(const vk::PhysicalDevice &Device);

  vk::SurfaceFormatKHR
  chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &Formats);

  vk::PresentModeKHR
  choosePresentMode(const std::vector<vk::PresentModeKHR> &Modes);

  vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR &Capabilities);

  vk::ResultValue<vk::UniqueShaderModule>
  createShaderModule(const uint32_t *Code, size_t CodeSize);

public:
  Engine(EngineCreateInfo CreateInfo);
  ~Engine();

  vk::Result initialize();
  vk::Result drawFrame();
  vk::Result waitIdle();
};

} // namespace engine
} // namespace vkmol

#endif // vk::MOL_ENGINE_H