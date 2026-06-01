#pragma once

#include <algorithm>
#include <cstdint>

namespace sw::core {

struct Position {
    uint32_t x;
    uint32_t y;

    friend bool operator==(const Position&, const Position&) = default;
};

// TODO: using Distance = StrongType

// Chebyshev (chessboard) distance between two cells — the metric used for range queries.
inline uint32_t chebyshev(Position a, Position b) {
    const uint32_t dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    const uint32_t dy = a.y > b.y ? a.y - b.y : b.y - a.y;
    return std::max(dx, dy);
}

}  // namespace sw::core
