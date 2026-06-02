#pragma once

#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <cstdint>

namespace sw::core::commands {

struct CreateMap {
    static constexpr const char* name = "CREATE_MAP";

    std::uint32_t width{};
    std::uint32_t height{};

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit("width", width);
        visitor.visit("height", height);
    }
};

struct March {
    static constexpr const char* name = "MARCH";

    UnitId unitId{};
    components::Position target{};

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit("unitId", unitId);
        visitor.visit("targetX", target.x);
        visitor.visit("targetY", target.y);
    }
};

}  // namespace sw::core::commands
