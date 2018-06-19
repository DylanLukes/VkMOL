#include <vkmol/renderer/ResourceHandle.h>
#include <vkmol/internal/renderer/ResourceContainer.h>

namespace vkmol {
namespace renderer {

template <class T>
ResourceContainer<T>::ResourceContainer()
: next(1) {}

template <class T>
std::pair<T &, ResourceHandle<T>> ResourceContainer<T>::add() {
    unsigned int handle = next++;

    auto result = resources.emplace(handle, T());
    assert(result.second);

    return std::make_pair(std::ref(result.first->second), ResourceHandle<T>(handle));
}

template <class T>
T &&ResourceContainer<T>::get(ResourceHandle<T> handle) {
    assert(handle.id != 0);

    auto it = resources.find(handle.id);
    assert(it != std::end(resources));

    return it->second;
}

template <class T>
void ResourceContainer<T>::remove(ResourceHandle<T> handle) {
    assert(handle.id != 0);

    auto it = resources.find(handle.id);
    assert(it != std::end(resources));

    resources.erase(it);
}

template <class T>
template <typename F>
void ResourceContainer<T>::removeWith(ResourceHandle<T> handle, F &&f) {
    assert(handle.id != 0);

    auto it = resources.find(handle.id);
    assert(it != std::end(resources));

    f(it->second);
    resources.erase(it);
}

template <class T>
template <typename F>
void ResourceContainer<T>::clearWith(F &&f) {
    auto it = std::begin(resources);
    while (it != std::end(resources)) {
        f(it->second);
        it = resources.erase(it);
    }
}

}; // namespace renderer
}; // namespace vkmol