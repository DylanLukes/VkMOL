#include <vulkan/vulkan.hpp>

#include "vkmol/debug.h"

#include <iostream>
#include <sstream>

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
    VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugReportCallbackEXT *pCallback) {
  auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugReportCallbackEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
    VkInstance instance,
    VkDebugReportCallbackEXT callback,
    const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugReportCallbackEXT");
  if (func != nullptr) {
    func(instance, callback, pAllocator);
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugReportMessageEXT(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char *pLayerPrefix,
    const char *pMessage,
    void *pUserData) {

  std::string prefix("");

  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) prefix += "ERROR:";
  if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) prefix += "WARNING:";
  if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) prefix += "INFO:";
  if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) prefix += "DEBUG:";
  if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    prefix += "PERFORMANCE:";

  std::stringstream debug;
  debug << prefix << " [" << pLayerPrefix << "] Code " << messageCode << " : " << pMessage;

  // TODO: id differently elsewhere?
  std::cerr << debug.str() << "\n";

  return VK_FALSE;
}