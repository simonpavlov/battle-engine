#pragma once

#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Features/Units/Hunter.hpp>
#include <Features/Units/Raven.hpp>
#include <Features/Units/Swordsman.hpp>
#include <cstdint>
#include <functional>
#include <vector>

namespace sw::graphics {

class WorldEditor {
public:
    void spawnSwordsman(sw::feature::commands::SpawnSwordsman cmd);
    void spawnHunter(sw::feature::commands::SpawnHunter cmd);
    void spawnRaven(sw::feature::commands::SpawnRaven cmd);

    void setStrength(sw::core::UnitId id, std::int32_t value);
    void setAgility(sw::core::UnitId id, std::int32_t value);
    void setRange(sw::core::UnitId id, std::uint32_t value);
    void setHealth(sw::core::UnitId id, std::int32_t hp, std::int32_t maxHp);
    void moveUnit(sw::core::UnitId id, sw::core::components::Position target);
    void setDestination(sw::core::UnitId id, sw::core::components::Position target);
    void clearDestination(sw::core::UnitId id);
    void deleteUnit(sw::core::UnitId id);

    void drain(sw::core::Engine& engine);

private:
    std::vector<std::function<void(sw::core::Engine&)>> edits_;
};

}  // namespace sw::graphics
