#include <vkmol/Engine.h>

namespace vkmol {

void Engine::draw() {}

void Engine::waitIdle() { vkDeviceWaitIdle(Device); }

void Engine::cleanup() {}

} // namespace vkmol