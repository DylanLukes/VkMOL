#include <utility>

#include <vulkan/vulkan.hpp>

#include "Debug.h"
#include <vkmol/Engine.h>

#include <algorithm>

#define RETURN_ON_FAILURE(Result)                                              \
  do {                                                                         \
    if (Result != vk::Result::eSuccess) {                                      \
      return Result;                                                           \
    }                                                                          \
  } while (0)

namespace vkmol {

Engine::Engine(const char *AppName, uint32_t AppVersion,
               std::vector<const char *> Extensions,
               std::vector<const char *> ValidationLayers,
               vkmol::SurfaceFactory SurfaceFactory)
    : AppName(AppName), AppVersion(AppVersion),
      Extensions(std::move(Extensions)),
      ValidationLayers(std::move(ValidationLayers)),
      SurfaceFactory(std::move(SurfaceFactory)) {

  // Some extensions are mandatory for all VkMOL engine instances.
  Extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

Engine::~Engine() {
  if (!ValidationLayers.empty()) {
    destroyDebugReportCallbackEXT(*Instance, Callback, nullptr);
  }
}

vk::Result Engine::setupInstance() {
  vk::ApplicationInfo AppInfo = {AppName, AppVersion, VKMOL_ENGINE_NAME,
                                 VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0};

  vk::InstanceCreateInfo CreateInfo = {
      vk::InstanceCreateFlags(),
      &AppInfo,
      static_cast<uint32_t>(ValidationLayers.size()),
      ValidationLayers.data(),
      static_cast<uint32_t>(Extensions.size()),
      Extensions.data()};

  auto [Result, Instance] = vk::createInstanceUnique(CreateInfo);
  this->Instance = std::move(Instance);
  return Result;
}

vk::Result Engine::setupDebugCallback() {
  if (ValidationLayers.empty()) {
    return vk::Result::eSuccess;
  }

  vk::DebugReportCallbackCreateInfoEXT CreateInfo = {
      vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eError,
      debugReportMessageEXT};

  // Cast to C-style, due to converage holes in vulkan-hpp.
  auto CInfo = VkDebugReportCallbackCreateInfoEXT(CreateInfo);

  return vk::Result(
      createDebugReportCallbackEXT(*Instance, &CInfo, nullptr, &Callback));
}

vk::Result Engine::setupPhysicalDevice() {
  auto [Result, PhysicalDevices] = Instance->enumeratePhysicalDevices();
  RETURN_ON_FAILURE(Result);

  if (PhysicalDevices.empty()) {
    return vk::Result::eErrorInitializationFailed;
  }

  auto SuitablePhysicalDevice =
      std::find_if(PhysicalDevices.begin(), PhysicalDevices.end(),
                   [](vk::PhysicalDevice Device) -> bool {
                     // TODO: check device capabilities.
                     return true;
                   });

  if (SuitablePhysicalDevice == PhysicalDevices.end()) {
    return vk::Result::eErrorInitializationFailed;
  }

  PhysicalDevice = *SuitablePhysicalDevice;
  return vk::Result::eSuccess;
}

vk::Result Engine::initialize() {
  vk::Result Result;

  Result = setupInstance();
  RETURN_ON_FAILURE(Result);

  Result = setupDebugCallback();
  RETURN_ON_FAILURE(Result);

  Result = setupPhysicalDevice();
  RETURN_ON_FAILURE(Result);

  return vk::Result::eSuccess;
}

void Engine::draw() {}

void Engine::cleanup() {}

} // namespace vkmol