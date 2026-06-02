#include "CombatSystem.hpp"

#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <Core/Modules/Vitals/HealthSystem.hpp>
#include <cassert>
#include <functional>
#include <optional>
#include <unordered_set>
#include <variant>

namespace sw::core {

namespace {

struct CoreCombatSystem : ICombatSystem {
    explicit CoreCombatSystem(Engine& engine) :
            engine(engine) {}

    Engine& engine;
    std::unordered_set<AttackKind> registered_kinds;

    void registerAttackKind(AttackKind kind) override {
        const bool inserted = registered_kinds.insert(kind).second;
        (void)inserted;
        assert(inserted && "attack kind already registered");
    }

    std::optional<std::reference_wrapper<IOnTargetReaction>> onTargetReactionOf(UnitId id) const {
        return engine.systems.getSystem<IUnitSystem>().getUnitType(id).findReaction<IOnTargetReaction>();
    }

    std::vector<UnitId> selectTargets(UnitId self, AttackKind kind, DistanceBand base) override {
        assert(registered_kinds.contains(kind) && "attack kind not registered");

        auto& position_system = engine.systems.getSystem<IPositionSystem>();
        auto& health_system = engine.systems.getSystem<IHealthSystem>();
        const components::Position self_position = position_system.getPosition(self);

        std::vector<UnitId> targets;
        for (const UnitId candidate : position_system.unitsInRange(self_position, Distance{0}, base.max, self)) {
            if (!health_system.has(candidate)) {
                continue;
            }

            DistanceBand band = base;
            if (auto reaction = onTargetReactionOf(candidate)) {
                const TargetResponse response = reaction->get().onTargeted(kind, base);
                if (std::holds_alternative<Untargetable>(response)) {
                    continue;
                }
                band = std::get<DistanceBand>(response);
                assert(band.max.value <= base.max.value && "reaction must not increase attack range beyond base.max");
            }

            const Distance distance = chebyshev(self_position, position_system.getPosition(candidate));
            if (distance.value >= band.min.value && distance.value <= band.max.value) {
                targets.push_back(candidate);
            }
        }
        return targets;
    }

    ~CoreCombatSystem() override = default;
};

}  // namespace

ICombatSystemPtr makeCoreCombatSystem(Engine& engine) {
    return std::make_unique<CoreCombatSystem>(engine);
}

}  // namespace sw::core
