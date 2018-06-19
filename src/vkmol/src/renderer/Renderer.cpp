#include <vkmol/internal/renderer/RendererImpl.h>
#include <vkmol/renderer/Renderer.h>

#include <cassert>

namespace vkmol {
namespace renderer {

Renderer::Renderer(const RendererInfo &info)
: impl(new RendererImpl(info)) {}

Renderer::Renderer(Renderer &&other)
: impl(std::move(other.impl)) {}

Renderer &Renderer::operator=(Renderer &&other) {
    if (this == &other) {
        return *this;
    }

    assert(!impl);

    impl = std::move(other.impl);
    return *this;
}

Renderer::~Renderer() = default;

}; // namespace renderer
}; // namespace vkmol
