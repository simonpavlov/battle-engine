#pragma once

#include <Core/Modules/Stats/Agility.hpp>
#include <Core/Modules/Spatial/CollisionReaction.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Foundation/Engine.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Stats/Speed.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Features/Actions/AttackAction.hpp>
#include <Features/Actions/MoveAction.hpp>
#include <cstdint>
#include <memory>

namespace sw::feature {

static const auto kRavenTypeId = core::UnitTypeId{.value = 13};

struct FlyingReaction : core::ICollisionReaction {
    bool blocksMovement() override {
        return false;
    }

    bool ignoresOccupants() override {
        return true;
    }
};

struct RavenTargetReaction : core::IOnTargetReaction {
    core::TargetResponse onTargeted(core::AttackKind kind, core::DistanceBand base) override {
        if (kind == core::kMeleeAttackKind) {
            return core::Untargetable{};
        }
        if (kind == core::kRangedAttackKind) {
            const auto dec = [](core::Distance d) {
                return core::Distance{d.value > 0 ? d.value - 1 : 0};
            };
            return core::DistanceBand{dec(base.min), dec(base.max)};
        }
        return base;
    }
};

inline core::UnitType makeRavenType(core::Engine& engine) {
    auto unit_type = core::UnitType{
        .id = kRavenTypeId,
        .name = "Raven",
        .actions = {},
        .reactions = {},
    };
    // Talon Strike: adjacent attack for Agility.
    unit_type.actions.push_back(std::make_unique<AttackAction>(engine, core::kMeleeAttackKind, [&engine](core::UnitId self) {
        const auto agility = engine.components.getComponent<core::Agility>().get(self).value;
        return core::AttackProperty{
            .band = {.min = core::Distance{1}, .max = core::Distance{1}},
            .damage = core::Damage{static_cast<int>(agility)},
        };
    }));
    unit_type.actions.push_back(std::make_unique<MoveAction>(engine));
    // Flight: moves over occupied cells without being blocked.
    unit_type.setReaction<core::ICollisionReaction>(std::make_unique<FlyingReaction>());
    // Evasion: immune to melee, and reduces an incoming ranged attack's reach by one.
    unit_type.setReaction<core::IOnTargetReaction>(std::make_unique<RavenTargetReaction>());
    return unit_type;
}

inline void spawnRaven(core::Engine& engine, core::UnitId id, core::Position pos, int hp, uint32_t agility) {
    engine.components.getComponent<core::Position>().add(id, std::move(pos));
    engine.components.getComponent<core::Health>().add(id, core::Health{hp});
    engine.components.getComponent<core::Speed>().add(id, core::Speed{2});
    engine.components.getComponent<core::Agility>().add(id, core::Agility{agility});
    engine.addUnit(kRavenTypeId, id);
}

}  // namespace sw::feature
