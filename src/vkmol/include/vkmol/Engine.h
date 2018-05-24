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
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#include <vkmol/Constants.h>
#include <vkmol/InstanceFactory.h>

namespace vkmol {

class Engine {
private:
  vk::Instance Instance;
  vk::DebugReportCallbackEXT Callback;
  vk::SurfaceKHR Surface;

  vk::PhysicalDevice PhysicalDevice = nullptr;
  vk::Device Device;

  vk::Queue GraphicsQueue;
  vk::Queue PresentQueue;

  vk::SwapchainKHR Swapchain;
  std::vector<vk::Image> SwapchainImages;
  vk::Format SwapchainFormat;
  vk::Extent2D SwapchainExtent;
  std::vector<vk::ImageView> SwapchainImageViews;
  std::vector<vk::Framebuffer> SwapchainFramebuffers;

  vk::RenderPass RenderPass;
  vk::DescriptorSetLayout DescriptorSetLayout;
  vk::PipelineLayout PipelineLayout;
  vk::Pipeline GraphicsPipeline;

  vk::CommandPool CommandPool;

  vk::Image DepthImage;
  vk::DeviceMemory DepthImageMemory;
  vk::ImageView DepthImageView;

  vk::Buffer VertexBuffer;
  vk::DeviceMemory VertexBufferMemory;
  vk::Buffer IndexBuffer;
  vk::DeviceMemory IndexBufferMemory;

  vk::Buffer UniformBuffer;
  vk::DeviceMemory UniformBufferMemory;

  vk::DescriptorPool DescriptorPool;
  vk::DescriptorSet DescriptorSet;

  std::vector<vk::CommandBuffer> CommandBuffers;

  std::vector<vk::Semaphore> ImageAvailableSemaphores;
  std::vector<vk::Semaphore> RenderFinishedSemaphores;
  std::vector<vk::Fence> InFlightFences;

  size_t CurrentFrame;

public:
  Engine(vk::Instance Instance, vk::SurfaceKHR Surface)
      : Instance(Instance), Surface(Surface) {}

  void setSurface(vk::SurfaceKHR Surface);

  void draw();
  void waitIdle();

  void cleanup();
};

} // namespace vkmol

#endif // vk::MOL_ENGINE_H