//
// Created by Dylan Lukes on 6/11/18.
//

#ifndef VKMOL_VERTEX_H
#define VKMOL_VERTEX_H

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkmol {
namespace engine {

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription();

  static std::array<vk::VertexInputAttributeDescription, 2>
  getAttributeDescriptions();
};

}; // namespace engine
}; // namespace vkmol

#endif // VKMOL_VERTEX_H
