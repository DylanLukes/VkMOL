#ifndef VKMOL_UNIFORM_H
#define VKMOL_UNIFORM_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkmol {
namespace engine {

struct UniformBufferObject {
  glm::mat4 Model;
  glm::mat4 View;
  glm::mat4 Projection;

  void dummy();
};

}; // namespace engine
}; // namespace vkmol

#endif //VKMOL_UNIFORM_H
