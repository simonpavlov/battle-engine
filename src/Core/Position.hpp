#pragma once

#include <cstdint>

namespace sw::core {

struct Position {
    uint32_t x;
    uint32_t y;

    friend bool operator==(const Position&, const Position&) = default;
};

}  // namespace sw::core
