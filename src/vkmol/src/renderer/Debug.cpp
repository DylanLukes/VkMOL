/*
  Copyright 2018, Dylan Lukes

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "vkmol/private/Debug.h"
#include <iostream>
#include <sstream>

PFN_vkCreateDebugReportCallbackEXT pfn_vkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT pfn_vkDestroyDebugReportCallbackEXT =
    nullptr;
PFN_vkDebugMarkerSetObjectNameEXT pfn_vkDebugMarkerSetObjectNameEXT = nullptr;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
    VkInstance                                instance,
    const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *             pAllocator,
    VkDebugReportCallbackEXT *                pCallback) {
    assert(pfn_vkCreateDebugReportCallbackEXT);
    return pfn_vkCreateDebugReportCallbackEXT(
        instance, pCreateInfo, pAllocator, pCallback);
}

VKAPI_ATTR void VKAPI_CALL
                vkDestroyDebugReportCallbackEXT(VkInstance                   instance,
                                                VkDebugReportCallbackEXT     callback,
                                                const VkAllocationCallbacks *pAllocator) {
    assert(pfn_vkDestroyDebugReportCallbackEXT);
    return pfn_vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectNameEXT(
    VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    assert(pfn_vkDebugMarkerSetObjectNameEXT);
    return pfn_vkDebugMarkerSetObjectNameEXT(device, pNameInfo);
}

VkBool32 debugCallbackFunc(VkDebugReportFlagsEXT      flags,
                           VkDebugReportObjectTypeEXT objectType,
                           uint64_t                   object,
                           size_t                     location,
                           int32_t                    messageCode,
                           const char *               pLayerPrefix,
                           const char *               pMessage,
                           void *                     pUserData) {

    std::string prefix;

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) prefix += "ERROR:";
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) prefix += "WARNING:";
    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) prefix += "INFO:";
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) prefix += "DEBUG:";
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        prefix += "PERFORMANCE:";

    std::stringstream debug;
    debug << prefix << " [" << pLayerPrefix << "] Code " << messageCode << " : "
          << pMessage;

    // TODO: handle differently elsewhere?
    std::cerr << debug.str() << "\n";

    return VK_FALSE;
}