#include "CombatSystem.hpp"

#include <Core/Engine.hpp>
#include <Core/HealthSystem.hpp>
#include <Core/PositionSystem.hpp>
#include <cassert>
#include <functional>
#include <limits>
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
        assert(inserted && "attack kind already registered");
    }

    std::optional<std::reference_wrapper<IOnTargetReaction>> onTargetReactionOf(UnitId id) {
        const auto it = engine.unit_to_type.find(id);
        if (it == engine.unit_to_type.end()) {
            return std::nullopt;
        }
        return it->second.get().findReaction<IOnTargetReaction>();
    }

    std::vector<UnitId> selectTargets(UnitId self, AttackKind kind, DistanceBand base) override {
        assert(registered_kinds.count(kind) && "attack kind not registered");

        auto& position_system = engine.systems.getSystem<IPositionSystem>();
        auto& health_system = engine.systems.getSystem<IHealthSystem>();
        const Position self_position = position_system.getPosition(self);

        std::vector<UnitId> targets;
        for (const UnitId candidate :
             position_system.unitsInRange(self_position, 0, std::numeric_limits<uint32_t>::max(), self)) {
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
            }

            const uint32_t distance = chebyshev(self_position, position_system.getPosition(candidate));
            if (distance >= band.min.value && distance <= band.max.value) {
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
