#include "HealthSystem.hpp"

#include <Core/Engine.hpp>
#include <Core/Health.hpp>

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

    void applyDamage(UnitId id, int amount) override {
        health().get(id).hp -= amount;
    }

    bool isAlive(UnitId id) override {
        return health().has(id) && health().get(id).hp > 0;
    }

    std::vector<UnitId> collectDead() override {
        std::vector<UnitId> dead;
        health().forEach([&](UnitId id, Health& value) {
            if (value.hp <= 0) {
                dead.push_back(id);
            }
        });
        return dead;
    }

    ~CoreHealthSystem() override = default;
};

}

IHealthSystemPtr MakeCoreHealthSystem(Engine& engine) {
    return std::make_unique<CoreHealthSystem>(engine);
}

}
