#ifndef VKMOL_RENDERER_RESOURCE_H
#define VKMOL_RENDERER_RESOURCE_H

#include <cstddef>
#include <variant>

#include "Buffer.h"

namespace vkmol {
namespace renderer {

using Resource = std::variant<Buffer>;

struct ResourceHasher final {
    size_t operator()(const Buffer &b) const {
        return b.getHash();
    }
};

}; // namespace renderer
}; // namespace vkmol

namespace std {

template <>
struct hash<vkmol::renderer::Resource> {
    size_t operator()(const vkmol::renderer::Resource &r) const {
        return std::visit(vkmol::renderer::ResourceHasher(), r);
    }
};
} // namespace std

#endif // VKMOL_RENDERER_RESOURCE_H
