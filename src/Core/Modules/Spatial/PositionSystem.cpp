#include "PositionSystem.hpp"

#include <Core/Foundation/Components.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Spatial/CollisionReaction.hpp>
#include <Core/Modules/Spatial/Destination.hpp>
#include <Core/Modules/Stats/Speed.hpp>

namespace sw::core {

namespace {

uint32_t stepToward(uint32_t from, uint32_t to) {
    if (to > from) {
        return from + 1;
    }
    if (to < from) {
        return from - 1;
    }
    return from;
}

class CorePositionSystem : public IPositionSystem {
public:
    CorePositionSystem(ComponentsLocator& components, SystemsLocator& systems) :
            components_(components),
            systems_(systems) {}

    void setBounds(uint32_t w, uint32_t h) override {
        width_ = w;
        height_ = h;
        mapCreated_.emit({w, h});
    }

    components::Position getPosition(UnitId id) override {
        return positions().get(id);
    }

    bool move(UnitId unit_to_move_id, components::Position target_position) override {
        const components::Position current = positions().get(unit_to_move_id);
        if (target_position == current) {
            return false;
        }

        if (target_position.x >= width_ || target_position.y >= height_) {
            return false;
        }

        if (!moverIgnoresOccupants(unit_to_move_id)) {
            bool blocked = false;
            positions().forEach([&](UnitId other_id, components::Position& other_position) {
                if (other_id == unit_to_move_id || other_position != target_position) {
                    return;
                }
                if (!isMoveAllowed(other_id)) {
                    blocked = true;
                }
            });
            if (blocked) {
                return false;
            }
        }

        positions().get(unit_to_move_id) = target_position;
        moved_.emit({unit_to_move_id, target_position});
        return true;
    }

    std::vector<UnitId> unitsInRange(
        components::Position center, Distance min_range, Distance max_range, UnitId exclude
    ) override {
        std::vector<UnitId> result;
        positions().forEach([&](UnitId id, components::Position& position) {
            if (id == exclude) {
                return;
            }
            const Distance distance = chebyshev(center, position);
            if (distance.value >= min_range.value && distance.value <= max_range.value) {
                result.push_back(id);
            }
        });
        return result;
    }

    void march(UnitId id, components::Position target) override {
        if (!positions().has(id)) {
            return;
        }
        marchStarted_.emit({id, positions().get(id), target});
        components_.getComponent<components::Destination>().add(
            id, components::Destination{.target = target, .active = true}
        );
    }

    bool advanceMarch(UnitId id) override {
        auto& destinations = components_.getComponent<components::Destination>();
        if (!destinations.has(id)) {
            return false;
        }
        components::Destination& destination = destinations.get(id);
        if (!destination.active) {
            return false;
        }

        if (positions().get(id) == destination.target) {
            destination.active = false;
            marchEnded_.emit({id, positions().get(id)});
            return false;
        }

        bool moved_any = false;
        for (uint32_t step = 0; step < speedOf(id); ++step) {
            const components::Position current = positions().get(id);
            if (current == destination.target) {
                break;
            }
            const components::Position next{
                .x = stepToward(current.x, destination.target.x),
                .y = stepToward(current.y, destination.target.y),
            };
            if (!move(id, next)) {
                break;
            }
            moved_any = true;
            if (positions().get(id) == destination.target) {
                destination.active = false;
                marchEnded_.emit({id, destination.target});
                break;
            }
        }
        return moved_any;
    }

    ~CorePositionSystem() override = default;

private:
    IComponentStore<components::Position>& positions() {
        return components_.getComponent<components::Position>();
    }

    uint32_t speedOf(UnitId id) {
        auto& speeds = components_.getComponent<components::Speed>();
        return speeds.has(id) ? speeds.get(id).value : 1;
    }

    std::optional<std::reference_wrapper<ICollisionReaction>> collisionReactionOf(UnitId id) const {
        return systems_.getSystem<IUnitSystem>().getUnitType(id).findReaction<ICollisionReaction>();
    }

    bool moverIgnoresOccupants(UnitId mover_id) const {
        auto reaction = collisionReactionOf(mover_id);
        return reaction && reaction->get().ignoresOccupants();
    }

    bool isMoveAllowed(UnitId occupant_id) const {
        auto reaction = collisionReactionOf(occupant_id);
        return !reaction || !reaction->get().blocksMovement();
    }

    ComponentsLocator& components_;
    SystemsLocator& systems_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

}  // namespace

IPositionSystemPtr makeCorePositionSystem(ComponentsLocator& components, SystemsLocator& systems) {
    return std::make_unique<CorePositionSystem>(components, systems);
}

}  // namespace sw::core
