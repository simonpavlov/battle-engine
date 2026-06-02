#pragma once

#include <Core/Foundation/StrongType.hpp>
#include <algorithm>
#include <cstdint>

namespace sw::core::components {

struct Position {
    uint32_t x;
    uint32_t y;

    friend bool operator==(const Position&, const Position&) = default;
};

}  // namespace sw::core::components

namespace sw::core {

using Distance = StrongType<uint32_t, struct DistanceTag>;

inline Distance chebyshev(components::Position a, components::Position b) {
    const uint32_t dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    const uint32_t dy = a.y > b.y ? a.y - b.y : b.y - a.y;
    return Distance{std::max(dx, dy)};
}

}  // namespace sw::core
