#pragma once

#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <string_view>

namespace sw::core::events {

struct Spawned {
    static constexpr const char* name = "UNIT_SPAWNED";

    UnitId unit_id;
    std::string_view type;
    components::Position pos;

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visitor.visit("unitId", unit_id);
        visitor.visit("unitType", type);
        visitor.visit("x", pos.x);
        visitor.visit("y", pos.y);
    }
};

struct Died {
    static constexpr const char* name = "UNIT_DIED";

    UnitId unit_id;

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visitor.visit("unitId", unit_id);
    }
};

}  // namespace sw::core::events
