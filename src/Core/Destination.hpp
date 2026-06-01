#pragma once

#include <Core/Position.hpp>

namespace sw::core {

// A MARCH order: where the unit is heading. `active` is cleared once it arrives, so the unit stops
// generating moves (which lets an idle tick end the run).
struct Destination {
    Position target;
    bool active;
};

}  // namespace sw::core
