#pragma once

#include <Core/Position.hpp>

namespace sw::core {

struct Destination {
    Position target;
    bool active;
};

}  // namespace sw::core
