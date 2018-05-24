#include <utility>

#include <vulkan/vulkan.hpp>

#include "Debug.h"
#include <vkmol/Engine.h>

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
  // TODO: destroy callback!
}

vk::Result Engine::setupDebugCallback() { return vk::Result::eSuccess; }

vk::Result Engine::createInstance() {
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

vk::Result Engine::initialize() { return vk::Result::eSuccess; }

void Engine::draw() {}

void Engine::cleanup() {}

} // namespace vkmol