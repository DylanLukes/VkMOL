#ifndef VKMOL_DEBUG_H
#define VKMOL_DEBUG_H

namespace vkmol {

VkResult createDebugReportCallbackEXT(
    VkInstance instance,
    const vk::DebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugReportCallbackEXT *pCallback);

void destroyDebugReportCallbackEXT(VkInstance instance,
                                   VkDebugReportCallbackEXT callback,
                                   const VkAllocationCallbacks *pAllocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
              uint64_t obj, size_t location, int32_t code,
              const char *layerPrefix, const char *msg, void *userData);

}

#endif // VKMOL_DEBUG_H