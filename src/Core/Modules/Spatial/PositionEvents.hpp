#pragma once

#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <cstdint>

namespace sw::core::events {

struct MapCreated {
    static constexpr const char* name = "MAP_CREATED";

    uint32_t width;
    uint32_t height;

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visitor.visit("width", width);
        visitor.visit("height", height);
    }
};

struct MarchStarted {
    static constexpr const char* name = "MARCH_STARTED";

    UnitId unit_id;
    components::Position from;
    components::Position to;

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visitor.visit("unitId", unit_id);
        visitor.visit("x", from.x);
        visitor.visit("y", from.y);
        visitor.visit("targetX", to.x);
        visitor.visit("targetY", to.y);
    }
};

struct Moved {
    static constexpr const char* name = "UNIT_MOVED";

    UnitId unit_id;
    components::Position pos;

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visitor.visit("unitId", unit_id);
        visitor.visit("x", pos.x);
        visitor.visit("y", pos.y);
    }
};

struct MarchEnded {
    static constexpr const char* name = "MARCH_ENDED";

    UnitId unit_id;
    components::Position pos;

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visitor.visit("unitId", unit_id);
        visitor.visit("x", pos.x);
        visitor.visit("y", pos.y);
    }
};

}  // namespace sw::core::events
