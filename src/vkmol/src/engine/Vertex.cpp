#include <vkmol/engine/vertex.h>

namespace vkmol {
namespace engine {

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
  vk::VertexInputBindingDescription bindingDescription = {};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = vk::VertexInputRate::eVertex;

  return bindingDescription;
}

std::array<vk::VertexInputAttributeDescription, 2>
Vertex::getAttributeDescriptions() {
  std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions =
      {};

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
  attributeDescriptions[0].offset = uint32_t(offsetof(Vertex, pos));

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
  attributeDescriptions[1].offset = uint32_t(offsetof(Vertex, color));

  return attributeDescriptions;
}

}; // namespace engine
}; // namespace vkmol