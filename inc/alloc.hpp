#pragma once

#include <memory_resource>
#include <type_traits>

#include <stuff/core/integers.hpp>

namespace john {

// trying really hard not to use zig rn

template<typename T>
[[nodiscard]] auto alloc(std::pmr::memory_resource* resource, usize n = 1) -> T* {
    auto* const ptr = resource->allocate(sizeof(T) * n, alignof(T));
    return reinterpret_cast<T*>(ptr);
}

template<typename T, typename... Args>
[[nodiscard]] auto create(std::pmr::memory_resource* resource, Args&&... args) -> T* {
    auto* const ptr = alloc<T>(resource, 1);
    std::construct_at(ptr, std::forward<Args>(args)...);
    return ptr;
}

template<typename T>
    requires std::is_default_constructible_v<T>
[[nodiscard]] auto create_n(std::pmr::memory_resource* resource, usize n) -> T* {
    auto* const ptr = alloc<T>(resource, n);
    for (auto i = 0uz; i < n; i++) {
        std::construct_at(ptr + i);
    }
    return ptr;
}

template<typename T>
auto alloc_at(std::pmr::memory_resource* resource, T*& ptr, usize n = 1) -> T* {
    return ptr = alloc<T>(resource, n);
}

template<typename T, typename... Args>
auto create_at(std::pmr::memory_resource* resource, T*& ptr, Args&&... args) -> T* {
    return std::construct_at(alloc_at(resource, ptr, 1), std::forward<Args>(args)...);
}

template<typename T>
void free(std::pmr::memory_resource* resource, T* ptr, usize n = 1) {
    resource->deallocate(static_cast<void*>(ptr), sizeof(T) * n, alignof(T));
}

template<typename T>
void destroy(std::pmr::memory_resource* resource, T* ptr, usize n = 1) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (usize i = 0; i < n; i++) {
            (&ptr[i])->~T();
        }
    }

    return free(resource, ptr, n);
}

}  // namespace john
