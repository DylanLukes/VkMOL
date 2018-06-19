#ifndef VKMOL_RENDERER_RESOURCE_H
#define VKMOL_RENDERER_RESOURCE_H

#include <cstddef>
#include <variant>


namespace vkmol {
namespace renderer {

struct Buffer;

using Resource = std::variant<Buffer>;

}; // namespace renderer
}; // namespace vkmol

namespace std {
template <>
struct hash<vkmol::renderer::Resource> {
    size_t operator()(const vkmol::renderer::Resource &r) const {

    }
};
} // namespace std

#endif // VKMOL_RENDERER_RESOURCE_H
