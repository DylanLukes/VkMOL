#ifndef VKMOL_MACROS_H
#define VKMOL_MACROS_H

#include <memory>
#include <tuple>

#include <vulkan/vulkan.hpp>

#define VKMOL_GUARD(__Result__)                                                \
  do {                                                                         \
    if ((__Result__) != vk::Result::eSuccess) {                                \
      return (__Result__);                                                     \
    }                                                                          \
  } while (0)

#define VKMOL_GUARD_VALUE(__Result__, __Value__)                               \
  do {                                                                         \
    if ((__Result__) != vk::Result::eSuccess) {                                \
      return {(__Result__), (__Value__)};                                      \
    }                                                                          \
  } while (0)

#if defined NDEBUG
#define TRACE(format, ...)
#else
#define TRACE(format, ...)                                                     \
  printf("%s::%s(%d) " format, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

// template <typename T>
// std::tuple<vk::Result, vk::UniqueHandle<T>>
// unwrap(typename vk::ResultValueType<T>::type ResultValue) {
//  std::tuple<vk::Result &, vk::UniqueHandle<T> &> X = ResultValue;
//  return {std::get<0>(X), std::move(std::get<1>(X))};
//}

namespace vkmol {

// Note: take's definition is split to pacify zealous IDE analysis.

template <typename T>
std::tuple<vk::Result, T> take(vk::ResultValue<T> ResultValue);

template <typename T>
std::tuple<vk::Result, T> take(vk::ResultValue<T> ResultValue) {
  return std::make_tuple(ResultValue.result, std::move(ResultValue.value));
}

} // namespace vkmol

#endif // VKMOL_MACROS_H
