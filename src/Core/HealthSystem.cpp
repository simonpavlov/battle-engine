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
        auto& hp = health().get(id).hp;
        if ((hp -= amount) <= 0) {
            engine.scheduleDeath(id);
        }
    }

    bool isAlive(UnitId id) override {
        return health().has(id) && health().get(id).hp > 0;
    }

    ~CoreHealthSystem() override = default;
};

}

IHealthSystemPtr MakeCoreHealthSystem(Engine& engine) {
    return std::make_unique<CoreHealthSystem>(engine);
}

}
