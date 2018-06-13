#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <vkmol/vkmol.h>

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

const int WIDTH = 1200;
const int HEIGHT = 800;

#ifdef NDEBUG
bool enableValidationLayers = false;
#else
bool enableValidationLayers = true;
#endif

vkmol::engine::Engine *Engine;

std::vector<const char *> getGLFWExtensions() {
  uint32_t GLFWExtensionCount = 0;
  const char **GLFWExtensions =
      glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);

  return std::vector<const char *>(GLFWExtensions,
                                   GLFWExtensions + GLFWExtensionCount);
}

void onError(int error, const char *description) {
  fprintf(stderr, "Error (%d): %s\n", error, description);
}

void onFramebufferSize(GLFWwindow *Window, int Width, int Height) {
  Engine->resize();
  Engine->drawFrame();
}

void onKey(GLFWwindow *Window, int Key, int Scancode, int Action, int Mods) {
  if (Key == GLFW_KEY_1 && Action == GLFW_PRESS) {
    Engine->setActivePipeline(vkmol::engine::PipelineIndex::Normal);
  }
  else if (Key == GLFW_KEY_2 && Action == GLFW_PRESS) {
    Engine->setActivePipeline(vkmol::engine::PipelineIndex::Wireframe);
  }
}

int main(int argc, char **argv) {
  // 0.0 - Initialize GLFW and create a window.
  glfwSetErrorCallback(onError);
  assert(glfwInit());

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  auto Window = glfwCreateWindow(WIDTH, HEIGHT, "Demo", nullptr, nullptr);
  glfwSetFramebufferSizeCallback(Window, onFramebufferSize);
  glfwSetKeyCallback(Window, onKey);

  auto InstanceExtensions = getGLFWExtensions();
  std::vector<const char *> DeviceExtensions;
  std::vector<const char *> ValidationLayers;

  // 0.2 - Enable debug reporting if needed.
  if (enableValidationLayers) {
    InstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    ValidationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
  }

  vkmol::engine::EngineCreateInfo CreateInfo;
  CreateInfo.AppName = "VkMol Demo";
  CreateInfo.AppVersion = VK_MAKE_VERSION(1, 0, 0);
  CreateInfo.InstanceExtensions = InstanceExtensions;
  CreateInfo.DeviceExtensions = DeviceExtensions;
  CreateInfo.ValidationLayers = ValidationLayers;
  CreateInfo.SurfaceFactory = [&](const vk::Instance &Instance) {
    VkSurfaceKHR Surf;

    auto Result = glfwCreateWindowSurface(Instance, Window, nullptr, &Surf);
    return vk::ResultValue<vk::SurfaceKHR>(vk::Result(Result), Surf);
  };
  CreateInfo.WindowSizeCallback = [&](void) {
    int Width, Height;
    glfwGetFramebufferSize(Window, &Width, &Height);
    return std::make_tuple(Width, Height);
  };


  Engine = new vkmol::engine::Engine(CreateInfo);

  auto Result = Engine->initialize();
  if (Result != vk::Result::eSuccess) {
    std::cerr << "Error: " << vk::to_string(Result) << "\n";
    throw std::runtime_error("Failed to initialize vkmol.");
  }

  // 4.0 – Main loop!
  while (!glfwWindowShouldClose(Window)) {
    glfwPollEvents();
    Result = Engine->drawFrame();

    if (Result != vk::Result::eSuccess) {
      std::cerr << "Error: " << vk::to_string(Result) << "\n";
    }
  }

  // 5.0 – Let everything finish up.
  Engine->waitIdle();

  delete Engine;

  glfwDestroyWindow(Window);
  glfwTerminate();
}
