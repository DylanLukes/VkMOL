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

namespace vkmol {

using SurfaceFactoryResult = typename vk::ResultValueType<vk::SurfaceKHR>::type;

using SurfaceFactory =
    typename std::function<vkmol::SurfaceFactoryResult(const vk::Instance &)>;

class Engine {
private:
  // Engine State
  // ------------

  vk::UniqueInstance Instance;
  vk::DebugReportCallbackEXT Callback;
  vk::UniqueSurfaceKHR Surface;

  vk::PhysicalDevice PhysicalDevice = nullptr;
  vk::UniqueDevice Device;

  vk::Queue GraphicsQueue;
  vk::Queue PresentQueue;

  vk::UniqueSwapchainKHR Swapchain;
  std::vector<vk::Image> SwapchainImages;
  vk::Format SwapchainFormat;
  vk::Extent2D SwapchainExtent;
  std::vector<vk::ImageView> SwapchainImageViews;
  std::vector<vk::Framebuffer> SwapchainFramebuffers;

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
  const char *AppName;
  uint32_t AppVersion;
  std::vector<const char *> Extensions;
  std::vector<const char *> ValidationLayers;

  SurfaceFactory SurfaceFactory;
  bool EnableValidationLayers;

  // Setup Routines
  // --------------
  vk::Result createInstance();
  vk::Result setupDebugCallback();

public:
  Engine(const char *AppName, uint32_t AppVersion,
         std::vector<const char *> Extensions,
         std::vector<const char *> ValidationLayers,
         vkmol::SurfaceFactory SurfaceFactory);
  ~Engine();

  vk::Result initialize();

  void draw();

  void cleanup();
};

} // namespace vkmol

#endif // vk::MOL_ENGINE_H