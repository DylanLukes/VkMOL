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

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <vkmol/vkmol.h>

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

const int WIDTH  = 1200;
const int HEIGHT = 800;

#ifdef NDEBUG
#else
bool enableDebug = true;
bool enableTrace = true;
#endif

vkmol::renderer::Renderer *Renderer;

std::vector<const char *> getGLFWExtensions() {
    uint32_t     GLFWExtensionCount = 0;
    const char **GLFWExtensions =
        glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);

    return std::vector<const char *>(GLFWExtensions,
                                     GLFWExtensions + GLFWExtensionCount);
}

void onError(int error, const char *description) {
    fprintf(stderr, "Error (%d): %s\n", error, description);
}

void onFramebufferSize(GLFWwindow *Window, int Width, int Height) {}

void onKey(GLFWwindow *Window, int Key, int Scancode, int Action, int Mods) {}

int main(int argc, char **argv) {
    // 0.0 - Initialize GLFW and create a window.
    glfwSetErrorCallback(onError);
    assert(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window = glfwCreateWindow(WIDTH, HEIGHT, "Demo", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, onFramebufferSize);
    glfwSetKeyCallback(window, onKey);

    auto InstanceExtensions = getGLFWExtensions();

    vkmol::renderer::RendererInfo rendererInfo;
    rendererInfo.appName    = "VkMOL Demo";
    rendererInfo.appVersion = VK_MAKE_VERSION(1, 0, 0);
    rendererInfo.debug      = enableDebug;
    rendererInfo.trace      = enableTrace;

    vkmol::renderer::RendererWSIDelegate rendererDelegate;
    rendererDelegate.getInstanceExtensions = [&]() {
        uint32_t     count;
        const char **extensions;

        extensions = glfwGetRequiredInstanceExtensions(&count);

        return std::vector<const char *>(extensions, extensions + count);
    };
    rendererDelegate.getSurface = [&](vk::Instance instance) {
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(instance, window, nullptr, &surface);
        return surface;
    };
    rendererDelegate.getWindowSize = [&]() {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        return std::make_tuple(width, height);
    };
    rendererDelegate.getFramebufferSize = [&]() {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        return std::make_tuple(width, height);
    };
    rendererInfo.delegate = rendererDelegate;


    Renderer = new vkmol::renderer::Renderer(rendererInfo);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    delete Renderer;

    glfwDestroyWindow(window);
    glfwTerminate();
}
