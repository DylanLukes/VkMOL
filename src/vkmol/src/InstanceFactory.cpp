#include <utility>

#include <vulkan/vulkan.hpp>

#include <vkmol/InstanceFactory.h>

#include <iostream>
#include <utility>

namespace vkmol {

InstanceFactory::InstanceFactory(std::vector<const char *> Extensions,
                                 std::vector<const char *> ValidationLayers)
    : RequiredExtensions(std::move(Extensions)),
      ValidationLayers(std::move(ValidationLayers)) {}

vk::ResultValueType<vk::Instance>::type
InstanceFactory::createInstance(const char *AppName) {
  vk::ApplicationInfo AppInfo(AppName, VK_MAKE_VERSION(1, 0, 0), "VkMOL",
                              VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

  vk::InstanceCreateInfo CreateInfo(
      vk::InstanceCreateFlags(), &AppInfo,
      static_cast<uint32_t>(ValidationLayers.size()), ValidationLayers.data(),
      static_cast<uint32_t>(RequiredExtensions.size()),
      RequiredExtensions.data());

  return vk::createInstance(CreateInfo);
}

} // namespace vkmol