#include "HealthSystem.hpp"

#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <algorithm>

namespace sw::core {

namespace {

struct CoreHealthSystem : IHealthSystem {
    explicit CoreHealthSystem(Engine& engine) :
            engine(engine) {}

    Engine& engine;

    IComponentStore<components::Health>& health() {
        return engine.components.getComponent<components::Health>();
    }

    bool has(UnitId id) override {
        return health().has(id);
    }

    std::int32_t getHp(UnitId id) override {
        return health().get(id).hp.value;
    }

    void applyDamage(UnitId source, UnitId target, std::int32_t amount) override {
        auto& hp = health().get(target).hp.value;
        const bool was_alive = hp > 0;
        hp -= amount;
        attacked.emit({source, target, Damage{amount}, components::HealthPoints{std::max(hp, 0)}});
        if (was_alive && hp <= 0) {
            engine.systems.getSystem<IUnitSystem>().scheduleDeath(target);
        }
    }

    bool isAlive(UnitId id) override {
        return health().has(id) && health().get(id).hp.value > 0;
    }

    ~CoreHealthSystem() override = default;
};

}  // namespace

IHealthSystemPtr makeCoreHealthSystem(Engine& engine) {
    return std::make_unique<CoreHealthSystem>(engine);
}

}  // namespace sw::core
