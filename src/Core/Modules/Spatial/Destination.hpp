#pragma once

#include <Core/Modules/Spatial/Position.hpp>

namespace sw::core {

struct Destination {
    Position target;
    bool active;
};

}  // namespace sw::core
