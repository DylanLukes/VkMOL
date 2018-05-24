#ifndef VKMOL_DEBUG_H
#define VKMOL_DEBUG_H

namespace vkmol {

VKAPI_ATTR VkResult VKAPI_CALL createDebugReportCallbackEXT(
    VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugReportCallbackEXT *pCallback);

void destroyDebugReportCallbackEXT(VkInstance instance,
                                   VkDebugReportCallbackEXT callback,
                                   const VkAllocationCallbacks *pAllocator);

VKAPI_ATTR void VKAPI_CALL destroyDebugReportCallbackEXT(
    VkInstance instance, VkDebugReportCallbackEXT callback,
    const VkAllocationCallbacks *pAllocator);

VKAPI_ATTR VkBool32 VKAPI_CALL debugReportMessageEXT(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
    uint64_t obj, size_t location, int32_t code, const char *layerPrefix,
    const char *msg, void *userData);

} // namespace vkmol

#endif // VKMOL_DEBUG_H