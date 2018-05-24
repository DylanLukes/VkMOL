#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <vkmol/vkmol.h>

#include <utility>
#include <vector>
#include <cstdio>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

const int WIDTH = 800;
const int HEIGHT = 600;

#ifdef NDEBUG
bool enableValidationLayers = false;
#else
bool enableValidationLayers = true;
#endif

void errorCallback(int error, const char* description)
{
  fprintf(stderr, "Error (%d): %s\n", error, description);
}

int main(int argc, char **argv) {
  // Temporarily.
  enableValidationLayers = false;

  // 0.0 - Get a window.
  glfwSetErrorCallback(errorCallback);
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  auto Window = glfwCreateWindow(WIDTH, HEIGHT, "Demo", nullptr, nullptr);

  // 0.1 - Obtain the required extensions from GLFW.
  uint32_t GLFWExtensionCount = 0;
  const char **GLFWExtensions =
      glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);

  std::vector<const char *> Extensions(GLFWExtensions,
                                       GLFWExtensions + GLFWExtensionCount);
  std::vector<const char *> ValidationLayers;

  // 0.2 - Enable debug reporting if needed.
  if (enableValidationLayers) {
    Extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    ValidationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
  }

  // 1.0 - Obtain an InstanceFactory for these extensions and vlayers and create
  // an instance.
  vkmol::InstanceFactory InstanceFactory(Extensions, ValidationLayers);

  auto [Result, Instance] = InstanceFactory.createInstance("Demo");
  if (Result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create instance.");
  }

  // 2.0 - Get the surface.
  VkSurfaceKHR Surface;
  auto CWSResult = glfwCreateWindowSurface(Instance, Window, nullptr, &Surface);
  if (CWSResult != VK_SUCCESS) {
    throw std::runtime_error("Failed to create surface.");
  }

  // 3.0 - Initialize an Engine with the Instance and Surface.
  vkmol::Engine Engine(Instance, Surface);

  // 4.0 – Main loop!
  while (!glfwWindowShouldClose(Window)) {
    glfwPollEvents();
    Engine.draw();
  }

  // 5.0 – Wait for idle and explicitly clean up.
  Engine.cleanup();
}

#pragma clang diagnostic pop