#include <vulkan/vulkan.hpp>

#include "../shaders/Shaders.h"
#include "./Utilities.h"
#include <vkmol/Debug.h>
#include <vkmol/engine/Engine.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <utility>

namespace vkmol {
namespace engine {

const std::vector<const char *> Engine::RequiredInstanceExtensions = {};

const std::vector<const char *> Engine::RequiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

Engine::Engine(EngineCreateInfo CreateInfo)
    : ApplicationInfo(
          CreateInfo.AppName,
          CreateInfo.AppVersion,
          VKMOL_ENGINE_NAME,
          VKMOL_ENGINE_VERSION,
          VK_API_VERSION_1_0),
      InstanceExtensions(std::move(CreateInfo.InstanceExtensions)),
      DeviceExtensions(std::move(CreateInfo.DeviceExtensions)),
      ValidationLayers(std::move(CreateInfo.ValidationLayers)),
      SurfaceFactory(std::move(CreateInfo.SurfaceFactory)),
      GetWindowSize(std::move(CreateInfo.WindowSizeCallback)) {

  InstanceExtensions.reserve(
      InstanceExtensions.size() + RequiredInstanceExtensions.size());

  InstanceExtensions.insert(
      InstanceExtensions.end(),
      RequiredInstanceExtensions.begin(),
      RequiredInstanceExtensions.end());

  DeviceExtensions.reserve(
      DeviceExtensions.size() + RequiredDeviceExtensions.size());

  DeviceExtensions.insert(
      DeviceExtensions.end(),
      RequiredDeviceExtensions.begin(),
      RequiredDeviceExtensions.end());
}

Engine::~Engine() {}

vk::Result Engine::setupInstance() {
  vk::Result Result;

  vk::InstanceCreateInfo CreateInfo;
  CreateInfo.flags = {};
  CreateInfo.pApplicationInfo = &ApplicationInfo;
  CreateInfo.enabledLayerCount = uint32_t(ValidationLayers.size());
  CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
  CreateInfo.enabledExtensionCount = uint32_t(InstanceExtensions.size());
  CreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

  std::tie(Result, Instance) = take(vk::createInstanceUnique(CreateInfo));
  return Result;
}

vk::Result Engine::setupSurface() {
  auto [Result, RawSurface] = SurfaceFactory(*Instance);
  GUARD_RESULT(Result);

  // Ensure the Surface is associated with the correct owner.
  auto Deleter = vk::UniqueHandleTraits<vk::SurfaceKHR>::deleter(*Instance);

  Surface = vk::UniqueSurfaceKHR(RawSurface, Deleter);
  return Result;
}

vk::Result Engine::setupDebugCallback() {
  vk::Result Result;

  if (ValidationLayers.empty()) {
    return vk::Result::eSuccess;
  }

  std::tie(Result, Callback) =
      take(Instance->createDebugReportCallbackEXTUnique(DebugReportCreateInfo));

  return Result;
}

vk::Result Engine::setupPhysicalDevice() {
  auto [EnumResult, Devices] = Instance->enumeratePhysicalDevices();
  GUARD_RESULT(EnumResult);

  if (Devices.empty()) {
    return vk::Result::eErrorInitializationFailed;
  }

  std::multimap<int, vk::PhysicalDevice> Candidates;

  for (const auto &Device : Devices) {
    auto [ScoreResult, Score] = scoreDevice(Device);
    GUARD_RESULT(ScoreResult);

    Candidates.insert({Score, Device});
  }

  auto [TopScore, BestDevice] = *Candidates.rbegin();

  if (TopScore < 0) {
    return vk::Result::eErrorInitializationFailed;
  }

  PhysicalDevice = BestDevice;

  return vk::Result::eSuccess;
}

vk::Result Engine::setupLogicalDevice() {
  auto [Result, Indices] = queryQueueFamilies(PhysicalDevice);
  GUARD_RESULT(Result);

  std::vector<vk::DeviceQueueCreateInfo> QueueCreateInfos;
  std::set<int> UniqueQueueFamilies = {Indices.GraphicsFamilyIndex,
                                       Indices.PresentFamilyIndex};

  float QueuePriority = 1.0f;

  for (int QueueFamily : UniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo QueueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        static_cast<uint32_t>(QueueFamily),
        1,
        &QueuePriority);
    QueueCreateInfos.push_back(QueueCreateInfo);
  }

  vk::PhysicalDeviceFeatures DeviceFeatures;

  vk::DeviceCreateInfo CreateInfo(
      vk::DeviceCreateFlags(),
      uint32_t(QueueCreateInfos.size()),
      QueueCreateInfos.data(),
      uint32_t(ValidationLayers.size()),
      ValidationLayers.data(),
      uint32_t(DeviceExtensions.size()),
      DeviceExtensions.data());

  std::tie(Result, Device) =
      take(PhysicalDevice.createDeviceUnique(CreateInfo));
  GUARD_RESULT(Result);

  GraphicsQueue =
      Device->getQueue(static_cast<uint32_t>(Indices.GraphicsFamilyIndex), 0);
  PresentQueue =
      Device->getQueue(static_cast<uint32_t>(Indices.PresentFamilyIndex), 0);

  return vk::Result::eSuccess;
}

vk::Result Engine::setupSwapchain() {
  vk::Result Result;
  vk::SwapchainCreateInfoKHR CreateInfo;

  SwapchainSupportDetails SwapchainSupport;
  std::tie(Result, SwapchainSupport) = querySwapchainSupport(PhysicalDevice);
  GUARD_RESULT(Result);

  auto SurfaceFormat = chooseSurfaceFormat(SwapchainSupport.Formats);
  auto PresentMode = choosePresentMode(SwapchainSupport.PresentModes);
  auto Extent = chooseExtent(SwapchainSupport.Capabilities);

  uint32_t MinImageCount = SwapchainSupport.Capabilities.minImageCount + 1;

  if (SwapchainSupport.Capabilities.maxImageCount > 0 &&
      MinImageCount > SwapchainSupport.Capabilities.maxImageCount) {
    MinImageCount = SwapchainSupport.Capabilities.maxImageCount;
  }

  QueueFamilyIndices Indices;
  std::tie(Result, Indices) = queryQueueFamilies(PhysicalDevice);
  GUARD_RESULT(Result);
  std::vector<uint32_t> QueueFamilyIndices(Indices);

  CreateInfo.surface = *Surface;
  CreateInfo.minImageCount = MinImageCount;
  CreateInfo.imageFormat = SurfaceFormat.format;
  CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
  CreateInfo.imageExtent = Extent;
  CreateInfo.imageArrayLayers = 1; // note: 2 for stereoscopic
  CreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

  if (Indices.GraphicsFamilyIndex != Indices.PresentFamilyIndex) {
    uint32_t QueueFamilyIndices[] = {(uint32_t)Indices.GraphicsFamilyIndex,
                                     (uint32_t)Indices.PresentFamilyIndex};

    CreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    CreateInfo.queueFamilyIndexCount = 2;
    CreateInfo.pQueueFamilyIndices = QueueFamilyIndices;
  } else {
    CreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
  }

  CreateInfo.preTransform = SwapchainSupport.Capabilities.currentTransform;
  CreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  CreateInfo.presentMode = PresentMode;
  CreateInfo.clipped = VK_TRUE; // should this always be on?

  std::tie(Result, Swapchain) =
      take(Device->createSwapchainKHRUnique(CreateInfo));
  GUARD_RESULT(Result);

  std::tie(Result, SwapchainImages) = Device->getSwapchainImagesKHR(*Swapchain);
  GUARD_RESULT(Result);

  SwapchainImageFormat = SurfaceFormat.format;
  SwapchainExtent = Extent;

  return vk::Result::eSuccess;
}

vk::Result Engine::setupImageViews() {
  vk::Result Result;
  SwapchainImageViews.resize(SwapchainImages.size());

  for (size_t I = 0; I < SwapchainImages.size(); ++I) {
    vk::ImageViewCreateInfo CreateInfo;
    CreateInfo.image = SwapchainImages[I];
    CreateInfo.viewType = vk::ImageViewType::e2D;
    CreateInfo.format = SwapchainImageFormat;
    CreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    CreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    CreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    CreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    CreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    CreateInfo.subresourceRange.baseMipLevel = 0;
    CreateInfo.subresourceRange.levelCount = 1;
    CreateInfo.subresourceRange.baseArrayLayer = 0;
    CreateInfo.subresourceRange.layerCount = 1;

    std::tie(Result, SwapchainImageViews[I]) =
        take(Device->createImageViewUnique(CreateInfo));
    GUARD_RESULT(Result);
  }

  return vk::Result::eSuccess;
}

vk::Result Engine::setupRenderPass() {
  vk::Result Result;

  vk::AttachmentDescription ColorAttachment;
  ColorAttachment.format = SwapchainImageFormat;
  ColorAttachment.samples = vk::SampleCountFlagBits::e1;
  ColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  ColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  ColorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  ColorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  ColorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  ColorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference ColorAttachmentRef;
  ColorAttachmentRef.attachment = 0;
  ColorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription Subpass;
  Subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  Subpass.colorAttachmentCount = 1;
  Subpass.pColorAttachments = &ColorAttachmentRef;

  vk::SubpassDependency Dependency;
  Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  Dependency.dstSubpass = 0;
  Dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  Dependency.srcAccessMask = vk::AccessFlags();
  Dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  Dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                             vk::AccessFlagBits::eColorAttachmentWrite;

  vk::RenderPassCreateInfo RenderPassInfo;
  RenderPassInfo.attachmentCount = 1;
  RenderPassInfo.pAttachments = &ColorAttachment;
  RenderPassInfo.subpassCount = 1;
  RenderPassInfo.pSubpasses = &Subpass;

  std::tie(Result, RenderPass) =
      take(Device->createRenderPassUnique(RenderPassInfo));
  GUARD_RESULT(Result);

  return vk::Result::eSuccess;
}

vk::Result Engine::setupGraphicsPipeline() {
  vk::Result Result;
  vk::UniqueShaderModule VertShaderModule, FragShaderModule;

  std::tie(Result, VertShaderModule) = take(createShaderModule(
      vkmol::shaders::VertSPIRV, sizeof(vkmol::shaders::VertSPIRV)));
  GUARD_RESULT(Result);

  std::tie(Result, FragShaderModule) = take(createShaderModule(
      vkmol::shaders::FragSPIRV, sizeof(vkmol::shaders::FragSPIRV)));
  GUARD_RESULT(Result);

  vk::PipelineShaderStageCreateInfo VertShaderStageInfo;
  VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
  VertShaderStageInfo.module = *VertShaderModule;
  VertShaderStageInfo.pName = "main";

  vk::PipelineShaderStageCreateInfo FragShaderStageInfo;
  FragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
  FragShaderStageInfo.module = *FragShaderModule;
  FragShaderStageInfo.pName = "main";

  vk::PipelineShaderStageCreateInfo ShaderStages[] = {VertShaderStageInfo,
                                                      FragShaderStageInfo};

  vk::PipelineVertexInputStateCreateInfo VertexInputInfo;
  VertexInputInfo.vertexBindingDescriptionCount = 0;
  VertexInputInfo.pVertexBindingDescriptions = nullptr;
  VertexInputInfo.vertexAttributeDescriptionCount = 0;
  VertexInputInfo.pVertexAttributeDescriptions = nullptr;

  vk::PipelineInputAssemblyStateCreateInfo InputAssemblyInfo;
  InputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
  InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  vk::Viewport Viewport;
  Viewport.x = 0.0f;
  Viewport.y = 0.0f;
  Viewport.width = (float)SwapchainExtent.width;
  Viewport.height = (float)SwapchainExtent.height;
  Viewport.minDepth = 0.0f;
  Viewport.maxDepth = 1.0f;

  vk::Rect2D Scissor;
  Scissor.offset = vk::Offset2D(0, 0);
  Scissor.extent = SwapchainExtent;

  vk::PipelineViewportStateCreateInfo ViewportInfo;
  ViewportInfo.viewportCount = 1;
  ViewportInfo.pViewports = &Viewport;
  ViewportInfo.scissorCount = 1;
  ViewportInfo.pScissors = &Scissor;

  vk::PipelineRasterizationStateCreateInfo RasterizationInfo;
  RasterizationInfo.depthClampEnable = VK_FALSE;
  RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  RasterizationInfo.polygonMode = vk::PolygonMode::eFill;
  RasterizationInfo.lineWidth = 1.0f;
  RasterizationInfo.cullMode = vk::CullModeFlagBits::eBack;
  RasterizationInfo.frontFace = vk::FrontFace::eCounterClockwise;
  RasterizationInfo.depthBiasEnable = VK_FALSE;

  vk::PipelineMultisampleStateCreateInfo MultisampleInfo;
  MultisampleInfo.sampleShadingEnable = VK_FALSE;
  MultisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
  MultisampleInfo.minSampleShading = 1.0f;
  MultisampleInfo.pSampleMask = nullptr;
  MultisampleInfo.alphaToCoverageEnable = VK_FALSE;
  MultisampleInfo.alphaToOneEnable = VK_FALSE;

  vk::PipelineColorBlendAttachmentState ColorBlendAttachmentState;
  ColorBlendAttachmentState.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  ColorBlendAttachmentState.blendEnable = VK_TRUE;
  ColorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
  ColorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eDstAlpha;
  ColorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
  ColorBlendAttachmentState.srcAlphaBlendFactor =
      vk::BlendFactor::eOneMinusSrcAlpha;
  ColorBlendAttachmentState.srcAlphaBlendFactor =
      vk::BlendFactor::eOneMinusSrcAlpha;
  ColorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;

  vk::PipelineColorBlendStateCreateInfo ColorBlendInfo;
  ColorBlendInfo.logicOpEnable = VK_FALSE;
  ColorBlendInfo.logicOp = vk::LogicOp::eCopy;
  ColorBlendInfo.attachmentCount = 1;
  ColorBlendInfo.pAttachments = &ColorBlendAttachmentState;
  ColorBlendInfo.blendConstants[0] = 0.0f;
  ColorBlendInfo.blendConstants[1] = 0.0f;
  ColorBlendInfo.blendConstants[2] = 0.0f;
  ColorBlendInfo.blendConstants[3] = 0.0f;

  vk::PipelineLayoutCreateInfo PipelineLayoutInfo;
  PipelineLayoutInfo.setLayoutCount = 0;
  PipelineLayoutInfo.pSetLayouts = nullptr;
  PipelineLayoutInfo.pushConstantRangeCount = 0;
  PipelineLayoutInfo.pPushConstantRanges = nullptr;

  std::tie(Result, PipelineLayout) =
      take(Device->createPipelineLayoutUnique(PipelineLayoutInfo));
  GUARD_RESULT(Result);

  vk::GraphicsPipelineCreateInfo PipelineInfo;
  PipelineInfo.stageCount = 2;
  PipelineInfo.pStages = ShaderStages;
  PipelineInfo.pVertexInputState = &VertexInputInfo;
  PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
  PipelineInfo.pViewportState = &ViewportInfo;
  PipelineInfo.pRasterizationState = &RasterizationInfo;
  PipelineInfo.pMultisampleState = &MultisampleInfo;
  PipelineInfo.pDepthStencilState = nullptr;
  PipelineInfo.pColorBlendState = &ColorBlendInfo;
  PipelineInfo.pDynamicState = nullptr;
  PipelineInfo.layout = *PipelineLayout;
  PipelineInfo.renderPass = *RenderPass;
  PipelineInfo.subpass = 0;
  PipelineInfo.basePipelineHandle = nullptr;
  PipelineInfo.basePipelineIndex = 0;

  std::tie(Result, GraphicsPipeline) = take(
      Device->createGraphicsPipelineUnique(vk::PipelineCache(), PipelineInfo));
  GUARD_RESULT(Result);

  return vk::Result::eSuccess;
}

vk::Result Engine::setupFramebuffers() {
  vk::Result Result;
  SwapchainFramebuffers.resize(SwapchainImageViews.size());

  for (size_t I = 0; I < SwapchainImageViews.size(); ++I) {
    vk::ImageView Attachments[] = {*SwapchainImageViews[I]};

    vk::FramebufferCreateInfo FramebufferInfo;
    FramebufferInfo.renderPass = *RenderPass;
    FramebufferInfo.attachmentCount = 1;
    FramebufferInfo.pAttachments = Attachments;

    FramebufferInfo.width = SwapchainExtent.width;
    FramebufferInfo.height = SwapchainExtent.height;
    FramebufferInfo.layers = 1;

    std::tie(Result, SwapchainFramebuffers[I]) =
        take(Device->createFramebufferUnique(FramebufferInfo));
    GUARD_RESULT(Result);
  }

  return vk::Result::eSuccess;
}

vk::Result Engine::setupCommandPool() {
  vk::Result Result;
  QueueFamilyIndices QueueFamilyIndices;

  std::tie(Result, QueueFamilyIndices) = queryQueueFamilies(PhysicalDevice);

  vk::CommandPoolCreateInfo PoolInfo;
  PoolInfo.queueFamilyIndex = uint32_t(QueueFamilyIndices.GraphicsFamilyIndex);

  std::tie(Result, CommandPool) =
      take(Device->createCommandPoolUnique(PoolInfo));
  GUARD_RESULT(Result);

  return vk::Result::eSuccess;
}

vk::Result Engine::setupCommandBuffers() {
  vk::Result Result;
  vk::CommandBufferAllocateInfo AllocInfo;

  AllocInfo.commandPool = *CommandPool;
  AllocInfo.level = vk::CommandBufferLevel::ePrimary;
  AllocInfo.commandBufferCount = 1;

  // Note: have to allocate and wrap in UniqueHandle manually here.
  // std::vector<vk::UniqueHandle<vk::T>> cannot be moved properly.
  std::vector<vk::CommandBuffer> Buffers;
  std::tie(Result, Buffers) = Device->allocateCommandBuffers(AllocInfo);
  GUARD_RESULT(Result);

  auto Deleter =
      vk::UniqueHandleTraits<vk::CommandBuffer>::deleter(*Device, *CommandPool);
  CommandBuffers.resize(Buffers.size());
  for (size_t I = 0; I < Buffers.size(); ++I) {
    CommandBuffers[I] = vk::UniqueCommandBuffer(Buffers[I], Deleter);
  }

  for (size_t I = 0; I < CommandBuffers.size(); ++I) {
    vk::CommandBufferBeginInfo BeginInfo;

    BeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    BeginInfo.pInheritanceInfo = nullptr;

    Result = CommandBuffers[I]->begin(BeginInfo);
    GUARD_RESULT(Result);

    vk::RenderPassBeginInfo RenderPassInfo;
    RenderPassInfo.renderPass = *RenderPass;
    RenderPassInfo.framebuffer = *SwapchainFramebuffers[I];
    RenderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    RenderPassInfo.renderArea.extent = SwapchainExtent;

    vk::ClearValue ClearColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
    RenderPassInfo.clearValueCount = 1;
    RenderPassInfo.pClearValues = &ClearColor;

    CommandBuffers[I]->beginRenderPass(
        RenderPassInfo, vk::SubpassContents::eInline);

    {
      CommandBuffers[I]->bindPipeline(
          vk::PipelineBindPoint::eGraphics, *GraphicsPipeline);

      CommandBuffers[I]->draw(3, 1, 0, 0);
    }

    CommandBuffers[I]->endRenderPass();

    Result = CommandBuffers[I]->end();
    GUARD_RESULT(Result);
  }

  return vk::Result::eSuccess;
}

vk::Result Engine::setupSyncObjects() {

}

vk::ResultValue<uint32_t>
Engine::scoreDevice(const vk::PhysicalDevice &Device) {
  vk::Result Result;
  uint32_t Score = 0;

  // Check queue families:
  QueueFamilyIndices QueueFamilyIndices;
  std::tie(Result, QueueFamilyIndices) = queryQueueFamilies(Device);
  GUARD_RESULT_VALUE(Result, Score);

  if (!QueueFamilyIndices.isComplete()) {
    return {vk::Result::eSuccess, 0};
  }

  // Check extensions:
  bool ExtensionsSupported;
  std::tie(Result, ExtensionsSupported) = checkDeviceExtensionSupport(Device);
  GUARD_RESULT_VALUE(Result, Score);

  if (!ExtensionsSupported) {
    return {vk::Result::eSuccess, 0};
  }

  // Check swapchain:
  SwapchainSupportDetails SwapchainSupport;
  std::tie(Result, SwapchainSupport) = querySwapchainSupport(Device);
  GUARD_RESULT_VALUE(Result, Score);

  if (SwapchainSupport.Formats.empty() ||
      SwapchainSupport.PresentModes.empty()) {
    return {vk::Result::eSuccess, 0};
  }

  // Can use feature support to zero undesirable devices.
  // auto Features = Device.getFeatures();

  // Can use properties to boost preferable devices.
  auto Properties = Device.getProperties();

  switch (Properties.deviceType) {
  case vk::PhysicalDeviceType::eDiscreteGpu:
    Score += 1000;
    break;
  case vk::PhysicalDeviceType::eIntegratedGpu:
    Score += 100;
    break;
  default:
    break;
  }

  // Can also score by limits and other properties here...
  // score += Properties.limits.maxWhatever

  return {vk::Result::eSuccess, Score};
}

vk::ResultValue<bool>
Engine::checkDeviceExtensionSupport(const vk::PhysicalDevice &Device) {
  auto [Result, AvailableExtensions] =
      Device.enumerateDeviceExtensionProperties();
  GUARD_RESULT_VALUE(Result, false);

  std::set<std::string> RequiredExtensions(
      DeviceExtensions.begin(), DeviceExtensions.end());

  for (const auto &Extension : AvailableExtensions) {
    RequiredExtensions.erase(Extension.extensionName);
  }

  return {vk::Result::eSuccess, RequiredExtensions.empty()};
}

vk::ResultValue<QueueFamilyIndices>
Engine::queryQueueFamilies(const vk::PhysicalDevice &Device) {
  QueueFamilyIndices Indices;

  auto QueueFamilyProperties = Device.getQueueFamilyProperties();

  uint32_t I = 0;
  for (const auto &QueueFamily : QueueFamilyProperties) {
    if (QueueFamily.queueCount > 0 &&
        QueueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      Indices.GraphicsFamilyIndex = I;
    }

    auto [Result, PresentSupported] = Device.getSurfaceSupportKHR(I, *Surface);
    GUARD_RESULT_VALUE(Result, Indices);

    if (QueueFamily.queueCount > 0 && PresentSupported) {
      Indices.PresentFamilyIndex = I;
    }

    if (Indices.isComplete()) {
      break;
    }

    ++I;
  }

  return {vk::Result::eSuccess, Indices};
}

vk::ResultValue<SwapchainSupportDetails>
Engine::querySwapchainSupport(const vk::PhysicalDevice &Device) {
  SwapchainSupportDetails Details;
  vk::Result Result;

  std::tie(Result, Details.Capabilities) =
      take(Device.getSurfaceCapabilitiesKHR(*Surface));
  GUARD_RESULT_VALUE(Result, Details);

  std::tie(Result, Details.Formats) =
      take(Device.getSurfaceFormatsKHR(*Surface));
  GUARD_RESULT_VALUE(Result, Details);

  std::tie(Result, Details.PresentModes) =
      take(Device.getSurfacePresentModesKHR(*Surface));
  GUARD_RESULT_VALUE(Result, Details);

  return {vk::Result::eSuccess, Details};
}

vk::SurfaceFormatKHR
Engine::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &Formats) {
  vk::SurfaceFormatKHR PreferredFormat = {vk::Format::eB8G8R8A8Unorm,
                                          vk::ColorSpaceKHR::eSrgbNonlinear};

  if ((Formats.size() == 1) &&
      (Formats.front().format == vk::Format::eUndefined)) {
    return PreferredFormat;
  }

  for (const auto &Format : Formats) {
    if (Format == PreferredFormat) {
      return Format;
    }
  }

  return Formats.front();
}

vk::PresentModeKHR
Engine::choosePresentMode(const std::vector<vk::PresentModeKHR> &Modes) {
  vk::PresentModeKHR PreferredMode = vk::PresentModeKHR::eFifo;

  for (const auto &Mode : Modes) {
    if (Mode == vk::PresentModeKHR::eMailbox) {
      return Mode;
    } else if (Mode == vk::PresentModeKHR::eImmediate) {
      PreferredMode = Mode;
    }
  }

  return PreferredMode;
}

vk::Extent2D
Engine::chooseExtent(const vk::SurfaceCapabilitiesKHR &Capabilities) {
  if (Capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return Capabilities.currentExtent;
  } else {
    auto [Width, Height] = GetWindowSize();

    vk::Extent2D ActualExtent = {static_cast<uint32_t>(Width),
                                 static_cast<uint32_t>(Height)};

    ActualExtent.width = std::max(
        Capabilities.minImageExtent.width,
        std::min(Capabilities.maxImageExtent.width, ActualExtent.width));
    ActualExtent.height = std::max(
        Capabilities.minImageExtent.height,
        std::min(Capabilities.maxImageExtent.height, ActualExtent.height));

    return ActualExtent;
  }
}

vk::ResultValue<vk::UniqueShaderModule>
Engine::createShaderModule(const uint32_t *Code, size_t CodeSize) {
  vk::ShaderModuleCreateInfo CreateInfo;

  CreateInfo.codeSize = CodeSize;
  CreateInfo.pCode = Code;

  return Device->createShaderModuleUnique(CreateInfo);
}

vk::Result Engine::initialize() {
  vk::Result Result;

  Result = setupInstance();
  GUARD_RESULT(Result);

  Result = setupSurface();
  GUARD_RESULT(Result);

  Result = setupDebugCallback();
  GUARD_RESULT(Result);

  Result = setupPhysicalDevice();
  GUARD_RESULT(Result);

  Result = setupLogicalDevice();
  GUARD_RESULT(Result);

  Result = setupSwapchain();
  GUARD_RESULT(Result);

  Result = setupImageViews();
  GUARD_RESULT(Result);

  Result = setupRenderPass();
  GUARD_RESULT(Result);

  Result = setupGraphicsPipeline();
  GUARD_RESULT(Result);

  Result = setupFramebuffers();
  GUARD_RESULT(Result);

  Result = setupCommandPool();
  GUARD_RESULT(Result);

  Result = setupCommandBuffers();
  GUARD_RESULT(Result);

  return vk::Result::eSuccess;
}

void Engine::drawFrame() {

}

} // namespace engine
} // namespace vkmol