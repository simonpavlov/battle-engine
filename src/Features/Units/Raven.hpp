#pragma once

#include <Core/Foundation/Components.hpp>
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

class FlyingReaction : public core::ICollisionReaction {
public:
    bool blocksMovement() override {
        return false;
    }

    bool ignoresOccupants() override {
        return true;
    }
};

class RavenTargetReaction : public core::IOnTargetReaction {
public:
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

inline core::UnitType makeRavenType(core::ComponentsLocator& components, core::SystemsLocator& systems) {
    auto unit_type = core::UnitType{
        .id = ravenTypeId(),
        .actions = {},
        .reactions = {},
    };
    // Talon Strike: adjacent attack for Agility.
    unit_type.addAction<AttackAction>(systems, core::kMeleeAttackKind, [&components](core::UnitId self) {
        const auto agility = components.getComponent<core::components::Agility>().get(self);
        return core::AttackProperty{
            .band = {.min = core::Distance{1}, .max = core::Distance{1}},
            .damage = core::Damage{agility.value},
        };
    });
    unit_type.addAction<MoveAction>(systems);
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

inline void spawnRaven(core::ComponentsLocator& components, core::SystemsLocator& systems, commands::SpawnRaven cmd) {
    components.getComponent<core::components::Position>().add(cmd.unitId, std::move(cmd.pos));
    components.getComponent<core::components::Health>().add(cmd.unitId, core::components::Health{cmd.hp, cmd.hp});
    components.getComponent<core::components::Speed>().add(cmd.unitId, core::components::Speed{2});
    components.getComponent<core::components::Agility>().add(cmd.unitId, std::move(cmd.agility));
    systems.getSystem<core::IUnitSystem>().addUnit(ravenTypeId(), cmd.unitId);
}

}  // namespace sw::feature
