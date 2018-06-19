#include <vkmol/renderer/ResourceHandle.h>

namespace vkmol {
namespace renderer {

template <class T>
ResourceHandle<T>::ResourceHandle(uint32_t handle)
: id(handle) {}

template <class T>
ResourceHandle<T>::ResourceHandle()
: id(0) {}

template <class T>
ResourceHandle<T>::ResourceHandle(const ResourceHandle<T> &other)
: id(other.id) {}

template <class T>
ResourceHandle<T> &ResourceHandle<T>::operator=(const ResourceHandle<T> &other) {
    id = other.id;

    return *this;
}

template <class T>
ResourceHandle<T>::ResourceHandle(ResourceHandle<T> &&other)
: id(other.id) {
    other.id = 0;
}

template <class T>
ResourceHandle<T> &ResourceHandle<T>::operator=(ResourceHandle<T> &&other) {
    id       = other.id;
    other.id = 0;

    return *this;
}

template <class T>
bool ResourceHandle<T>::operator==(const ResourceHandle<T> &other) const {
    return id == other.id;
}

template <class T>
ResourceHandle<T>::operator bool() const {
    return id != 0;
}

}; // namespace renderer
}; // namespace vkmol