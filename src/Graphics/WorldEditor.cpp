#include "WorldEditor.hpp"

#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Combat/Range.hpp>
#include <Core/Modules/Spatial/Destination.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <Core/Modules/Stats/Agility.hpp>
#include <Core/Modules/Stats/Strength.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <utility>

namespace sw::graphics {

void WorldEditor::spawnSwordsman(sw::feature::commands::SpawnSwordsman cmd) {
    edits_.emplace_back([cmd](sw::core::Engine& engine) {
        if (engine.components.getComponent<sw::core::components::Position>().has(cmd.unitId)) {
            return;
        }
        sw::feature::spawnSwordsman(engine.components, engine.systems, cmd);
    });
}

void WorldEditor::spawnHunter(sw::feature::commands::SpawnHunter cmd) {
    edits_.emplace_back([cmd](sw::core::Engine& engine) {
        if (engine.components.getComponent<sw::core::components::Position>().has(cmd.unitId)) {
            return;
        }
        sw::feature::spawnHunter(engine.components, engine.systems, cmd);
    });
}

void WorldEditor::spawnRaven(sw::feature::commands::SpawnRaven cmd) {
    edits_.emplace_back([cmd](sw::core::Engine& engine) {
        if (engine.components.getComponent<sw::core::components::Position>().has(cmd.unitId)) {
            return;
        }
        sw::feature::spawnRaven(engine.components, engine.systems, cmd);
    });
}

void WorldEditor::setStrength(sw::core::UnitId id, std::int32_t value) {
    edits_.emplace_back([id, value](sw::core::Engine& engine) {
        auto& strength = engine.components.getComponent<sw::core::components::Strength>();
        if (strength.has(id)) {
            strength.get(id).value = value;
        }
    });
}

void WorldEditor::setAgility(sw::core::UnitId id, std::int32_t value) {
    edits_.emplace_back([id, value](sw::core::Engine& engine) {
        auto& agility = engine.components.getComponent<sw::core::components::Agility>();
        if (agility.has(id)) {
            agility.get(id).value = value;
        }
    });
}

void WorldEditor::setRange(sw::core::UnitId id, std::uint32_t value) {
    edits_.emplace_back([id, value](sw::core::Engine& engine) {
        auto& range = engine.components.getComponent<sw::core::components::Range>();
        if (range.has(id)) {
            range.get(id).value = value;
        }
    });
}

void WorldEditor::setHealth(sw::core::UnitId id, std::int32_t hp, std::int32_t maxHp) {
    edits_.emplace_back([id, hp, maxHp](sw::core::Engine& engine) {
        auto& health = engine.components.getComponent<sw::core::components::Health>();
        if (health.has(id)) {
            auto& h = health.get(id);
            h.hp.value = hp;
            h.max_hp.value = maxHp;
        }
    });
}

void WorldEditor::moveUnit(sw::core::UnitId id, sw::core::components::Position target) {
    edits_.emplace_back([id, target](sw::core::Engine& engine) {
        engine.systems.getSystem<sw::core::IPositionSystem>().move(id, target);
    });
}

void WorldEditor::setDestination(sw::core::UnitId id, sw::core::components::Position target) {
    edits_.emplace_back([id, target](sw::core::Engine& engine) {
        auto& destinations = engine.components.getComponent<sw::core::components::Destination>();
        if (destinations.has(id)) {
            auto& destination = destinations.get(id);
            destination.target = target;
            destination.active = true;
            return;
        }
        engine.systems.getSystem<sw::core::IPositionSystem>().march(id, target);
    });
}

void WorldEditor::clearDestination(sw::core::UnitId id) {
    edits_.emplace_back([id](sw::core::Engine& engine) {
        auto& destinations = engine.components.getComponent<sw::core::components::Destination>();
        if (destinations.has(id)) {
            destinations.get(id).active = false;
        }
    });
}

void WorldEditor::deleteUnit(sw::core::UnitId id) {
    edits_.emplace_back([id](sw::core::Engine& engine) {
        engine.systems.getSystem<sw::core::IUnitSystem>().scheduleDeath(id);
    });
}

void WorldEditor::drain(sw::core::Engine& engine) {
    for (auto& edit : edits_) {
        edit(engine);
    }
    edits_.clear();
}

}  // namespace sw::graphics
