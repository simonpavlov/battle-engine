#pragma once

#include <cstddef>
#include <functional>

namespace sw::core {

template <class T, class Tag>
struct StrongType {
    T value;

    friend bool operator==(const StrongType&, const StrongType&) = default;
};

}  // namespace sw::core

template <class T, class Tag>
struct std::hash<sw::core::StrongType<T, Tag>> {
    std::size_t operator()(const sw::core::StrongType<T, Tag>& key) const noexcept {
        return std::hash<T>{}(key.value);
    }
};
