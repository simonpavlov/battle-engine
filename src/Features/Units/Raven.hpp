#pragma once

#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Modules/Spatial/CollisionReaction.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Stats/Agility.hpp>
#include <Core/Modules/Stats/Speed.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <Features/Actions/AttackAction.hpp>
#include <Features/Actions/MoveAction.hpp>
#include <memory>

namespace sw::feature {

inline const core::UnitTypeId& ravenTypeId() {
    static const core::UnitTypeId id{"Raven"};
    return id;
}

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
        .id = ravenTypeId(),
        .actions = {},
        .reactions = {},
    };
    // Talon Strike: adjacent attack for Agility.
    unit_type.actions.push_back(
        std::make_unique<AttackAction>(engine, core::kMeleeAttackKind, [&engine](core::UnitId self) {
            const auto agility = engine.components.getComponent<core::components::Agility>().get(self);
            return core::AttackProperty{
                .band = {.min = core::Distance{1}, .max = core::Distance{1}},
                .damage = core::Damage{agility.value},
            };
        })
    );
    unit_type.actions.push_back(std::make_unique<MoveAction>(engine));
    // Flight: moves over occupied cells without being blocked.
    unit_type.setReaction<core::ICollisionReaction>(std::make_unique<FlyingReaction>());
    // Evasion: immune to melee, and reduces an incoming ranged attack's reach by one.
    unit_type.setReaction<core::IOnTargetReaction>(std::make_unique<RavenTargetReaction>());
    return unit_type;
}

namespace commands {

struct SpawnRaven {
    static constexpr const char* name = "SPAWN_RAVEN";

    core::UnitId unitId{};
    core::components::Position pos{};
    core::components::HealthPoints hp{};
    core::components::Agility agility{};

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit("unitId", unitId);
        visitor.visit("x", pos.x);
        visitor.visit("y", pos.y);
        visitor.visit("hp", hp);
        visitor.visit("agility", agility);
    }
};

}  // namespace commands

inline void spawnRaven(core::Engine& engine, commands::SpawnRaven cmd) {
    engine.components.getComponent<core::components::Position>().add(cmd.unitId, std::move(cmd.pos));
    engine.components.getComponent<core::components::Health>().add(
        cmd.unitId, core::components::Health{cmd.hp, cmd.hp}
    );
    engine.components.getComponent<core::components::Speed>().add(cmd.unitId, core::components::Speed{2});
    engine.components.getComponent<core::components::Agility>().add(cmd.unitId, std::move(cmd.agility));
    engine.systems.getSystem<core::IUnitSystem>().addUnit(ravenTypeId(), cmd.unitId);
}

}  // namespace sw::feature
