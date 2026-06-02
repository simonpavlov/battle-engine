#include "UnitSystem.hpp"

#include <Core/Foundation/Components.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <cassert>
#include <functional>
#include <utility>
#include <vector>

namespace sw::core {

namespace {

class CoreUnitSystem : public IUnitSystem {
public:
    explicit CoreUnitSystem(ComponentsLocator& components) :
            components_(components) {}

    void registerUnitType(UnitType&& unit_type) override {
        const auto [it, inserted] = unit_types_.emplace(unit_type.id, std::move(unit_type));
        (void)it;
        (void)inserted;
        assert(inserted && "unit type already registered for this id");
    }

    void addUnit(UnitTypeId unit_type_id, UnitId unit_id) override {
        auto& type = unit_types_.at(unit_type_id);
        const auto [it, inserted] = unit_to_type_.emplace(unit_id, std::ref(type));
        (void)it;
        (void)inserted;
        assert(inserted && "unit already exists");
        creation_order_.push_back(unit_id);
        spawned_.emit({unit_id, type.id.value, components_.getComponent<components::Position>().get(unit_id)});
    }

    void scheduleDeath(UnitId unit_id) override {
        pending_deaths_.push_back(unit_id);
    }

    void sweep() override {
        for (const UnitId unit_id : pending_deaths_) {
            delUnit(unit_id);
        }
        pending_deaths_.clear();
    }

    bool stepActions() override {
        bool acted = false;
        for (const UnitId unit_id : creation_order_) {
            const auto it = unit_to_type_.find(unit_id);
            if (it == unit_to_type_.end()) {
                continue;
            }
            for (const auto& action : it->second.get().actions) {
                if (action->tryExecute(unit_id)) {
                    acted = true;
                    break;
                }
            }
        }
        return acted;
    }

    std::size_t aliveCount() const override {
        return creation_order_.size();
    }

    const UnitType& getUnitType(UnitId unit_id) const override {
        const auto it = unit_to_type_.find(unit_id);
        assert(it != unit_to_type_.end() && "not found unit type");
        return it->second.get();
    }

    ~CoreUnitSystem() override = default;

private:
    void delUnit(UnitId unit_id) {
        if (!unit_to_type_.contains(unit_id)) {
            return;
        }
        died_.emit({unit_id});
        unit_to_type_.erase(unit_id);
        std::erase(creation_order_, unit_id);
        components_.removeUnitEverywhere(unit_id);
    }

    ComponentsLocator& components_;
    UnitTypes unit_types_;
    UnitToType unit_to_type_;
    std::vector<UnitId> creation_order_;
    std::vector<UnitId> pending_deaths_;
};

}  // namespace

IUnitSystemPtr makeCoreUnitSystem(ComponentsLocator& components) {
    return std::make_unique<CoreUnitSystem>(components);
}

}  // namespace sw::core
