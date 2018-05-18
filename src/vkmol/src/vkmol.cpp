#include "vkmol/vkmol.h"
#include <vulkan/vulkan.h>

int foo() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  return extensionCount;
}