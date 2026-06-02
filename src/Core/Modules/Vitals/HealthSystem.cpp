#include "HealthSystem.hpp"

#include <Core/Foundation/Components.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <algorithm>

namespace sw::core {

namespace {

class CoreHealthSystem : public IHealthSystem {
public:
    CoreHealthSystem(ComponentsLocator& components, SystemsLocator& systems) :
            components_(components),
            systems_(systems) {}

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
        attacked_.emit({source, target, Damage{amount}, components::HealthPoints{std::max(hp, 0)}});
        if (was_alive && hp <= 0) {
            systems_.getSystem<IUnitSystem>().scheduleDeath(target);
        }
    }

    bool isAlive(UnitId id) override {
        return health().has(id) && health().get(id).hp.value > 0;
    }

    ~CoreHealthSystem() override = default;

private:
    IComponentStore<components::Health>& health() {
        return components_.getComponent<components::Health>();
    }

    ComponentsLocator& components_;
    SystemsLocator& systems_;
};

}  // namespace

IHealthSystemPtr makeCoreHealthSystem(ComponentsLocator& components, SystemsLocator& systems) {
    return std::make_unique<CoreHealthSystem>(components, systems);
}

}  // namespace sw::core
