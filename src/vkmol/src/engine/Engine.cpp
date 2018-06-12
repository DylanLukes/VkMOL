#include <vulkan/vulkan.hpp>

#if !defined(NDEBUG) && defined(__APPLE__)
#include <MoltenVK/vk_mvk_moltenvk.h>
#endif

#include "../shaders/Shaders.h"
#include "./Utilities.h"
#include <vkmol/Debug.h>
#include <vkmol/engine/Engine.h>
#include <vkmol/engine/Uniform.h>
#include <vkmol/engine/Vertex.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <utility>

namespace vkmol {
namespace engine {

const std::vector<Vertex> TestVertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                          {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                          {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

const std::vector<const char *> Engine::RequiredInstanceExtensions = {};

const std::vector<const char *> Engine::RequiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

Engine::Engine(EngineCreateInfo CreateInfo)
    : ApplicationInfo(CreateInfo.AppName,
                      CreateInfo.AppVersion,
                      VKMOL_ENGINE_NAME,
                      VKMOL_ENGINE_VERSION,
                      VK_API_VERSION_1_0),
      InstanceExtensions(std::move(CreateInfo.InstanceExtensions)),
      DeviceExtensions(std::move(CreateInfo.DeviceExtensions)),
      ValidationLayers(std::move(CreateInfo.ValidationLayers)),
      SurfaceFactory(std::move(CreateInfo.SurfaceFactory)),
      GetWindowSize(std::move(CreateInfo.WindowSizeCallback)) {

  InstanceExtensions.reserve(InstanceExtensions.size() +
                             RequiredInstanceExtensions.size());

  InstanceExtensions.insert(InstanceExtensions.end(),
                            RequiredInstanceExtensions.begin(),
                            RequiredInstanceExtensions.end());

  DeviceExtensions.reserve(DeviceExtensions.size() +
                           RequiredDeviceExtensions.size());

  DeviceExtensions.insert(DeviceExtensions.end(),
                          RequiredDeviceExtensions.begin(),
                          RequiredDeviceExtensions.end());
}

Engine::~Engine() {}

vk::Result Engine::createInstance() {
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

vk::Result Engine::createSurface() {
  auto [Result, RawSurface] = SurfaceFactory(*Instance);
  VKMOL_GUARD(Result);

  // Ensure the Surface is associated with the correct owner.
  auto Deleter = vk::UniqueHandleTraits<vk::SurfaceKHR>::deleter(*Instance);

  Surface = vk::UniqueSurfaceKHR(RawSurface, Deleter);
  return Result;
}

vk::Result Engine::installDebugCallback() {
  vk::Result Result;

  if (ValidationLayers.empty()) {
    return vk::Result::eSuccess;
  }

  std::tie(Result, Callback) =
      take(Instance->createDebugReportCallbackEXTUnique(DebugReportCreateInfo));

  return Result;
}

vk::Result Engine::createPhysicalDevice() {
  vk::Result Result;
  std::vector<vk::PhysicalDevice> Devices;

  std::tie(Result, Devices) = Instance->enumeratePhysicalDevices();
  VKMOL_GUARD(Result);

  if (Devices.empty()) {
    return vk::Result::eErrorInitializationFailed;
  }

  std::multimap<int, vk::PhysicalDevice> Candidates;

  for (const auto &Device : Devices) {
    uint32_t Score;
    std::tie(Result, Score) = scoreDevice(Device);
    VKMOL_GUARD(Result);

    Candidates.insert({Score, Device});
  }

  auto [TopScore, BestDevice] = *Candidates.rbegin();

  if (TopScore < 0) {
    return vk::Result::eErrorInitializationFailed;
  }

  PhysicalDevice = BestDevice;

  return vk::Result::eSuccess;
}

vk::Result Engine::createLogicalDevice() {
  auto [Result, Indices] = queryQueueFamilies(PhysicalDevice);
  VKMOL_GUARD(Result);

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

  vk::DeviceCreateInfo CreateInfo(vk::DeviceCreateFlags(),
                                  uint32_t(QueueCreateInfos.size()),
                                  QueueCreateInfos.data(),
                                  uint32_t(ValidationLayers.size()),
                                  ValidationLayers.data(),
                                  uint32_t(DeviceExtensions.size()),
                                  DeviceExtensions.data());

  std::tie(Result, Device) =
      take(PhysicalDevice.createDeviceUnique(CreateInfo));
  VKMOL_GUARD(Result);

  GraphicsQueue =
      Device->getQueue(static_cast<uint32_t>(Indices.GraphicsFamilyIndex), 0);
  PresentQueue =
      Device->getQueue(static_cast<uint32_t>(Indices.PresentFamilyIndex), 0);

#if !defined(NDEBUG) && defined(__APPLE__)
  MVKDeviceConfiguration MVKConfig;
  vkGetMoltenVKDeviceConfigurationMVK(*Device, &MVKConfig);
  MVKConfig.debugMode = VK_TRUE;
  MVKConfig.performanceTracking = VK_TRUE;
  MVKConfig.performanceLoggingFrameCount = 300;
  vkSetMoltenVKDeviceConfigurationMVK(*Device, &MVKConfig);
#endif

  return vk::Result::eSuccess;
}

vk::Result Engine::createSwapchain() {
  vk::Result Result;
  vk::SwapchainCreateInfoKHR CreateInfo;

  SwapchainSupportDetails SwapchainSupport;
  std::tie(Result, SwapchainSupport) = querySwapchainSupport(PhysicalDevice);
  VKMOL_GUARD(Result);

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
  VKMOL_GUARD(Result);
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
  VKMOL_GUARD(Result);

  std::tie(Result, SwapchainImages) = Device->getSwapchainImagesKHR(*Swapchain);
  VKMOL_GUARD(Result);

  SwapchainImageFormat = SurfaceFormat.format;
  SwapchainExtent = Extent;

  return vk::Result::eSuccess;
}

vk::Result Engine::createImageViews() {
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
    VKMOL_GUARD(Result);
  }

  return vk::Result::eSuccess;
}

vk::Result Engine::createRenderPass() {
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
  VKMOL_GUARD(Result);

  return vk::Result::eSuccess;
}

vk::Result Engine::createGraphicsPipeline() {
  vk::Result Result;
  vk::UniqueShaderModule VertShaderModule, FragShaderModule;

  auto VertShaderCode = vkmol::shaders::V1VertSPIRV;
  auto VertShaderSize = sizeof(vkmol::shaders::V1VertSPIRV);
  auto FragShaderCode = vkmol::shaders::V1FragSPIRV;
  auto FragShaderSize = sizeof(vkmol::shaders::V1FragSPIRV);

  std::tie(Result, VertShaderModule) =
      take(createShaderModule(VertShaderCode, VertShaderSize));
  VKMOL_GUARD(Result);

  std::tie(Result, FragShaderModule) =
      take(createShaderModule(FragShaderCode, FragShaderSize));
  VKMOL_GUARD(Result);

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

  auto BindingDescription = Vertex::getBindingDescription();
  auto AttributeDescriptions = Vertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo VertexInputInfo;
  VertexInputInfo.vertexBindingDescriptionCount = 1;
  VertexInputInfo.pVertexBindingDescriptions = &BindingDescription;
  VertexInputInfo.vertexAttributeDescriptionCount =
      uint32_t(AttributeDescriptions.size());
  VertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();

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
  RasterizationInfo.frontFace = vk::FrontFace::eClockwise;
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
  VKMOL_GUARD(Result);

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
  VKMOL_GUARD(Result);

  return vk::Result::eSuccess;
}

vk::Result Engine::createFramebuffers() {
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
    VKMOL_GUARD(Result);
  }

  return vk::Result::eSuccess;
}

vk::Result Engine::createVertexBuffer() {
  vk::Result Result;
  vk::DeviceSize BufferSize = sizeof(TestVertices[0]) * TestVertices.size();

  std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory> StagingBufferAlloc;
  std::tie(Result, StagingBufferAlloc) =
      take(createBuffer(BufferSize,
                        vk::BufferUsageFlagBits::eTransferSrc,
                        vk::MemoryPropertyFlagBits::eHostVisible |
                            vk::MemoryPropertyFlagBits::eHostCoherent));

  void *Data;
  Device->mapMemory(*std::get<1>(StagingBufferAlloc),
                    0,
                    BufferSize,
                    vk::MemoryMapFlags(),
                    &Data);
  memcpy(Data, TestVertices.data(), size_t(BufferSize));
  Device->unmapMemory(*std::get<1>(StagingBufferAlloc));

  std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory> VertexBufferAlloc;
  std::tie(Result, VertexBufferAlloc) =
      take(createBuffer(BufferSize,
                        vk::BufferUsageFlagBits::eTransferDst |
                            vk::BufferUsageFlagBits::eVertexBuffer,
                        vk::MemoryPropertyFlagBits::eDeviceLocal));

  this->VertexBuffer = std::move(std::get<0>(VertexBufferAlloc));
  this->VertexBufferMemory = std::move(std::get<1>(VertexBufferAlloc));

  Result =
      copyBuffer(*std::get<0>(StagingBufferAlloc), *VertexBuffer, BufferSize);

  return vk::Result::eSuccess;
}

vk::Result Engine::createIndexBuffer() { return vk::Result::eSuccess; }

vk::Result Engine::createCommandPool() {
  vk::Result Result;
  QueueFamilyIndices QueueFamilyIndices;

  std::tie(Result, QueueFamilyIndices) = queryQueueFamilies(PhysicalDevice);

  vk::CommandPoolCreateInfo PoolInfo;
  PoolInfo.queueFamilyIndex = uint32_t(QueueFamilyIndices.GraphicsFamilyIndex);

  std::tie(Result, CommandPool) =
      take(Device->createCommandPoolUnique(PoolInfo));
  VKMOL_GUARD(Result);

  return vk::Result::eSuccess;
}

vk::Result Engine::createCommandBuffers() {
  vk::Result Result;
  vk::CommandBufferAllocateInfo AllocInfo;

  AllocInfo.commandPool = *CommandPool;
  AllocInfo.level = vk::CommandBufferLevel::ePrimary;
  AllocInfo.commandBufferCount = uint32_t(SwapchainFramebuffers.size());

  // Note: have to allocate and wrap in UniqueHandle manually here.
  // std::vector<vk::UniqueHandle<vk::T>> cannot be moved properly.
  std::vector<vk::CommandBuffer> Buffers;
  std::tie(Result, Buffers) = Device->allocateCommandBuffers(AllocInfo);
  VKMOL_GUARD(Result);

  CommandBuffers.reserve(SwapchainFramebuffers.size());
  vk::UniqueHandleTraits<vk::CommandBuffer>::deleter Deleter(*Device,
                                                             *CommandPool);

  for (size_t I = 0; I < SwapchainFramebuffers.size(); ++I) {
    CommandBuffers.push_back(vk::UniqueCommandBuffer(Buffers[I], Deleter));
  }

  for (size_t I = 0; I < CommandBuffers.size(); ++I) {
    vk::CommandBufferBeginInfo BeginInfo;

    BeginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
    BeginInfo.pInheritanceInfo = nullptr;

    Result = CommandBuffers[I]->begin(BeginInfo);
    VKMOL_GUARD(Result);

    vk::RenderPassBeginInfo RenderPassInfo;
    RenderPassInfo.renderPass = *RenderPass;
    RenderPassInfo.framebuffer = *SwapchainFramebuffers[I];
    RenderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    RenderPassInfo.renderArea.extent = SwapchainExtent;

    vk::ClearValue ClearColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
    RenderPassInfo.clearValueCount = 1;
    RenderPassInfo.pClearValues = &ClearColor;

    CommandBuffers[I]->beginRenderPass(RenderPassInfo,
                                       vk::SubpassContents::eInline);

    CommandBuffers[I]->bindPipeline(vk::PipelineBindPoint::eGraphics,
                                    *GraphicsPipeline);

    vk::Buffer VertexBuffers[] = {*VertexBuffer};
    vk::DeviceSize Offsets[] = {0};
    CommandBuffers[I]->bindVertexBuffers(0, 1, VertexBuffers, Offsets);

    CommandBuffers[I]->draw(uint32_t(TestVertices.size()), 1, 0, 0);

    CommandBuffers[I]->endRenderPass();

    Result = CommandBuffers[I]->end();
    VKMOL_GUARD(Result);
  }

  return vk::Result::eSuccess;
}

vk::Result Engine::createSyncObjects() {
  vk::Result Result;
  ImageAvailableSemaphores.resize(MaxFramesInFlight);
  RenderFinishedSemaphores.resize(MaxFramesInFlight);
  InFlightFences.resize(MaxFramesInFlight);

  vk::SemaphoreCreateInfo SemaphoreInfo;
  vk::FenceCreateInfo FenceInfo;
  FenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

  for (size_t I = 0; I < MaxFramesInFlight; ++I) {
    std::tie(Result, ImageAvailableSemaphores[I]) =
        take(Device->createSemaphoreUnique(SemaphoreInfo));
    VKMOL_GUARD(Result);

    std::tie(Result, RenderFinishedSemaphores[I]) =
        take(Device->createSemaphoreUnique(SemaphoreInfo));
    VKMOL_GUARD(Result);

    std::tie(Result, InFlightFences[I]) =
        take(Device->createFenceUnique(FenceInfo));
    VKMOL_GUARD(Result);
  }

  return vk::Result::eSuccess;
}

vk::ResultValue<uint32_t>
Engine::scoreDevice(const vk::PhysicalDevice &Device) {
  vk::Result Result;
  uint32_t Score = 0;

  // Check queue families:
  QueueFamilyIndices QueueFamilyIndices;
  std::tie(Result, QueueFamilyIndices) = queryQueueFamilies(Device);
  VKMOL_GUARD_VALUE(Result, Score);

  if (!QueueFamilyIndices.isComplete()) {
    return {vk::Result::eSuccess, 0};
  }

  // Check extensions:
  bool ExtensionsSupported;
  std::tie(Result, ExtensionsSupported) = checkDeviceExtensionSupport(Device);
  VKMOL_GUARD_VALUE(Result, Score);

  if (!ExtensionsSupported) {
    return {vk::Result::eSuccess, 0};
  }

  // Check swapchain:
  SwapchainSupportDetails SwapchainSupport;
  std::tie(Result, SwapchainSupport) = querySwapchainSupport(Device);
  VKMOL_GUARD_VALUE(Result, Score);

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
  VKMOL_GUARD_VALUE(Result, false);

  std::set<std::string> RequiredExtensions(DeviceExtensions.begin(),
                                           DeviceExtensions.end());

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
    VKMOL_GUARD_VALUE(Result, Indices);

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
  VKMOL_GUARD_VALUE(Result, Details);

  std::tie(Result, Details.Formats) =
      take(Device.getSurfaceFormatsKHR(*Surface));
  VKMOL_GUARD_VALUE(Result, Details);

  std::tie(Result, Details.PresentModes) =
      take(Device.getSurfacePresentModesKHR(*Surface));
  VKMOL_GUARD_VALUE(Result, Details);

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
    }
    //    else if (Mode == vk::PresentModeKHR::eImmediate) {
    //      PreferredMode = Mode;
    //    }
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

vk::ResultValue<uint32_t>
Engine::queryMemoryType(uint32_t TypeFilter,
                        vk::MemoryPropertyFlags Properties) {
  auto MemProperties = PhysicalDevice.getMemoryProperties();

  for (uint32_t i = 0; i < MemProperties.memoryTypeCount; i++) {
    if ((TypeFilter & (1 << i)) && (MemProperties.memoryTypes[i].propertyFlags &
                                    Properties) == Properties) {
      return {vk::Result::eSuccess, i};
    }
  }

  return {vk::Result::eErrorOutOfDeviceMemory, 0}; // todo: not the right error
}

vk::ResultValue<std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory>>
Engine::createBuffer(vk::DeviceSize Size,
                     vk::BufferUsageFlags UsageFlags,
                     vk::MemoryPropertyFlags MemoryFlags) {
  vk::Result Result;
  std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory> BufferMemory;

  vk::BufferCreateInfo BufferInfo;
  BufferInfo.size = Size;
  BufferInfo.usage = UsageFlags;
  BufferInfo.sharingMode = vk::SharingMode::eExclusive;

  std::tie(Result, std::get<0>(BufferMemory)) =
      take(Device->createBufferUnique(BufferInfo));
  VKMOL_GUARD_VALUE(Result, std::move(BufferMemory));

  auto MemRequirements =
      Device->getBufferMemoryRequirements(*std::get<0>(BufferMemory));

  uint32_t MemoryTypeIndex;
  std::tie(Result, MemoryTypeIndex) =
      queryMemoryType(MemRequirements.memoryTypeBits, MemoryFlags);
  VKMOL_GUARD_VALUE(Result, std::move(BufferMemory));

  vk::MemoryAllocateInfo AllocInfo;
  AllocInfo.allocationSize = MemRequirements.size;
  AllocInfo.memoryTypeIndex = MemoryTypeIndex;

  std::tie(Result, std::get<1>(BufferMemory)) =
      take(Device->allocateMemoryUnique(AllocInfo));
  VKMOL_GUARD_VALUE(Result, std::move(BufferMemory));

  Device->bindBufferMemory(
      *std::get<0>(BufferMemory), *std::get<1>(BufferMemory), 0);

  return {vk::Result::eSuccess, std::move(BufferMemory)};
}

vk::Result Engine::copyBuffer(vk::Buffer SrcBuffer,
                              vk::Buffer DstBuffer,
                              vk::DeviceSize Size) {
  vk::Result Result;

  vk::CommandBufferAllocateInfo AllocInfo;
  AllocInfo.level = vk::CommandBufferLevel::ePrimary;
  AllocInfo.commandPool = *CommandPool;
  AllocInfo.commandBufferCount = 1;

  std::vector<vk::CommandBuffer> RawBuffers;
  vk::UniqueHandleTraits<vk::CommandBuffer>::deleter Deleter(*Device,
                                                             *CommandPool);
  std::tie(Result, RawBuffers) =
      take(Device->allocateCommandBuffers(AllocInfo));
  VKMOL_GUARD(Result);
  vk::UniqueCommandBuffer CommandBuffer(RawBuffers.front(), Deleter);

  vk::CommandBufferBeginInfo BeginInfo;
  BeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  CommandBuffer->begin(BeginInfo);

  vk::BufferCopy CopyRegion;
  CopyRegion.size = Size;
  CommandBuffer->copyBuffer(SrcBuffer, DstBuffer, 1, &CopyRegion);

  CommandBuffer->end();

  vk::SubmitInfo SubmitInfo;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &*CommandBuffer;

  GraphicsQueue.submit(1, &SubmitInfo, nullptr);
  GraphicsQueue.waitIdle();

  return vk::Result::eSuccess;
}

vk::Result Engine::recreateSwapchain() {
  printf("%s\n", "Recreating swapchain...");

  vk::Result Result;
  Device->waitIdle();

  Result = createSwapchain();
  VKMOL_GUARD(Result);

  Result = createImageViews();
  VKMOL_GUARD(Result);

  Result = createRenderPass();
  VKMOL_GUARD(Result);

  Result = createGraphicsPipeline();
  VKMOL_GUARD(Result);

  Result = createFramebuffers();
  VKMOL_GUARD(Result);

  Result = createCommandBuffers();
  VKMOL_GUARD(Result);

  return vk::Result::eSuccess;
}

vk::Result Engine::initialize() {
  vk::Result Result;

  Result = createInstance();
  VKMOL_GUARD(Result);

  Result = createSurface();
  VKMOL_GUARD(Result);

  Result = installDebugCallback();
  VKMOL_GUARD(Result);

  Result = createPhysicalDevice();
  VKMOL_GUARD(Result);

  Result = createLogicalDevice();
  VKMOL_GUARD(Result);

  Result = createSwapchain();
  VKMOL_GUARD(Result);

  Result = createImageViews();
  VKMOL_GUARD(Result);

  Result = createRenderPass();
  VKMOL_GUARD(Result);

  Result = createGraphicsPipeline();
  VKMOL_GUARD(Result);

  Result = createFramebuffers();
  VKMOL_GUARD(Result);

  Result = createCommandPool();
  VKMOL_GUARD(Result);

  Result = createVertexBuffer();
  VKMOL_GUARD(Result);

  Result = createIndexBuffer();
  VKMOL_GUARD(Result);

  Result = createCommandBuffers();
  VKMOL_GUARD(Result);

  Result = createSyncObjects();
  VKMOL_GUARD(Result);

  return vk::Result::eSuccess;
}

vk::Result Engine::drawFrame() {
  vk::Result Result;
  auto MaxTimeOut = std::numeric_limits<uint64_t>::max();

  Result = Device->waitForFences(
      1, &*InFlightFences[CurrentFrame], VK_TRUE, MaxTimeOut);
  VKMOL_GUARD(Result);

  Result = Device->resetFences(1, &*InFlightFences[CurrentFrame]);
  VKMOL_GUARD(Result);

  uint32_t ImageIndex;
  Result = Device->acquireNextImageKHR(*Swapchain,
                                       MaxTimeOut,
                                       *ImageAvailableSemaphores[CurrentFrame],
                                       nullptr,
                                       &ImageIndex);
  if (Result == vk::Result::eErrorOutOfDateKHR ||
      Result == vk::Result::eSuboptimalKHR) {
    recreateSwapchain();
  }
  VKMOL_GUARD(Result);

  vk::SubmitInfo SubmitInfo;
  vk::Semaphore WaitSemaphores[] = {*ImageAvailableSemaphores[CurrentFrame]};
  vk::PipelineStageFlags WaitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};

  SubmitInfo.waitSemaphoreCount = 1;
  SubmitInfo.pWaitSemaphores = WaitSemaphores;
  SubmitInfo.pWaitDstStageMask = WaitStages;

  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &*CommandBuffers[ImageIndex];

  vk::Semaphore SignalSemaphores[] = {*RenderFinishedSemaphores[CurrentFrame]};
  SubmitInfo.signalSemaphoreCount = 1;
  SubmitInfo.pSignalSemaphores = SignalSemaphores;

  Result = GraphicsQueue.submit(1, &SubmitInfo, *InFlightFences[CurrentFrame]);
  VKMOL_GUARD(Result);

  vk::PresentInfoKHR PresentInfo;
  PresentInfo.waitSemaphoreCount = 1;
  PresentInfo.pWaitSemaphores = SignalSemaphores;

  vk::SwapchainKHR Swapchains[] = {*Swapchain};
  PresentInfo.swapchainCount = 1;
  PresentInfo.pSwapchains = Swapchains;
  PresentInfo.pImageIndices = &ImageIndex;

  Result = PresentQueue.presentKHR(&PresentInfo);
  if (Result == vk::Result::eErrorOutOfDateKHR ||
      Result == vk::Result::eSuboptimalKHR) {
    recreateSwapchain();
  }
  VKMOL_GUARD(Result);

  CurrentFrame = (CurrentFrame + 1) % MaxFramesInFlight;
  return vk::Result::eSuccess;
}

vk::Result Engine::waitIdle() { return Device->waitIdle(); }

} // namespace engine
} // namespace vkmol