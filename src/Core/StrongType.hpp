#pragma once

#include <cstddef>
#include <functional>

namespace sw::core {

template <class T>
struct StrongType {
    T value;

    friend bool operator==(const StrongType&, const StrongType&) = default;
};

}  // namespace sw::core

template <class T>
struct std::hash<sw::core::StrongType<T>> {
    std::size_t operator()(const sw::core::StrongType<T>& key) const noexcept {
        return std::hash<T>{}(key.value);
    }
};
