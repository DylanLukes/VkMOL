#include <vulkan/vulkan.hpp>

#include "Utilities.h"
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
          VK_MAKE_VERSION(1, 0, 0),
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

  vk::DebugReportCallbackCreateInfoEXT CreateInfo;
  CreateInfo.flags =
      vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning;
  // vk::DebugReportFlagBitsEXT::eDebug;
  CreateInfo.pfnCallback = debugReportMessageEXT;

  std::tie(Result, Callback) =
      take(Instance->createDebugReportCallbackEXTUnique(CreateInfo));

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

  QueueFamilyIndices QueueFamilyIndices;
  std::tie(Result, QueueFamilyIndices) = queryQueueFamilies(PhysicalDevice);
  GUARD_RESULT(Result);

  CreateInfo.flags = {};

  return vk::Result::eSuccess;
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

  return vk::Result::eSuccess;
}

void Engine::draw() {}

} // namespace engine
} // namespace vkmol