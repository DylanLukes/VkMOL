#ifndef VKMOL_INSTANCE_FACTORY_H
#define VKMOL_INSTANCE_FACTORY_H

#include <vulkan/vulkan.hpp>

#include <vector>

#include <vkmol/Constants.h>

namespace vkmol {

class InstanceFactory {
private:
  std::vector<const char *> RequiredExtensions;
  std::vector<const char *> ValidationLayers;

  bool checkValidationLayerSupport();

public:
  InstanceFactory(std::vector<const char *> Extensions,
                  std::vector<const char *> ValidationLayers);

  vk::ResultValueType<vk::Instance>::type
  createInstance(const char *AppName);
};

} // namespace vkmol

#endif // VKMOL_INSTANCE_FACTORY_H