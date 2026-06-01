#include "HealthSystem.hpp"

#include <Core/Foundation/Engine.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <algorithm>

namespace sw::core {

namespace {

struct CoreHealthSystem : IHealthSystem {
    explicit CoreHealthSystem(Engine& engine) :
            engine(engine) {}

    Engine& engine;

    IComponentStore<Health>& health() {
        return engine.components.getComponent<Health>();
    }

    bool has(UnitId id) override {
        return health().has(id);
    }

    int getHp(UnitId id) override {
        return health().get(id).hp;
    }

    void applyDamage(UnitId source, UnitId target, int amount) override {
        auto& hp = health().get(target).hp;
        const bool was_alive = hp > 0;
        hp -= amount;
        attacked.emit(source, target, amount, std::max(hp, 0));
        if (was_alive && hp <= 0) {
            engine.scheduleDeath(target);
        }
    }

    bool isAlive(UnitId id) override {
        return health().has(id) && health().get(id).hp > 0;
    }

    ~CoreHealthSystem() override = default;
};

}  // namespace

IHealthSystemPtr makeCoreHealthSystem(Engine& engine) {
    return std::make_unique<CoreHealthSystem>(engine);
}

}  // namespace sw::core
