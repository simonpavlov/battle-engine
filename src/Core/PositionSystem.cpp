#include "PositionSystem.hpp"

#include <Core/CollisionReaction.hpp>
#include <Core/Destination.hpp>
#include <Core/Engine.hpp>
#include <Core/Speed.hpp>

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

struct CorePositionSystem : IPositionSystem {
    explicit CorePositionSystem(Engine& engine) :
            engine(engine) {}

    Engine& engine;
    uint32_t width = 0;
    uint32_t height = 0;

    IComponentStore<Position>& positions() {
        return engine.components.getComponent<Position>();
    }

    void setBounds(uint32_t w, uint32_t h) override {
        width = w;
        height = h;
        mapCreated.emit(w, h);
    }

    Position getPosition(UnitId id) override {
        return positions().get(id);
    }

    bool move(UnitId unit_to_move_id, Position target_position) override {
        const Position current = positions().get(unit_to_move_id);
        if (target_position == current) {
            return false;
        }

        if (target_position.x >= width || target_position.y >= height) {
            return false;
        }

        if (!moverIgnoresOccupants(unit_to_move_id)) {
            bool blocked = false;
            positions().forEach([&](UnitId other_id, Position& other_position) {
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
        moved.emit(unit_to_move_id, target_position);
        return true;
    }

    std::vector<UnitId> unitsInRange(Position center, uint32_t min_range, uint32_t max_range, UnitId exclude) override {
        std::vector<UnitId> result;
        positions().forEach([&](UnitId id, Position& position) {
            if (id == exclude) {
                return;
            }
            const uint32_t distance = chebyshev(center, position);
            if (distance >= min_range && distance <= max_range) {
                result.push_back(id);
            }
        });
        return result;
    }

    void march(UnitId id, Position target) override {
        if (!positions().has(id)) {
            return;
        }
        marchStarted.emit(id, positions().get(id), target);
        engine.components.getComponent<Destination>().add(id, Destination{target, true});
    }

    bool advanceMarch(UnitId id) override {
        auto& destinations = engine.components.getComponent<Destination>();
        if (!destinations.has(id)) {
            return false;
        }
        Destination& destination = destinations.get(id);
        if (!destination.active) {
            return false;
        }

        if (positions().get(id) == destination.target) {
            destination.active = false;
            marchEnded.emit(id, positions().get(id));
            return false;
        }

        bool moved_any = false;
        for (uint32_t step = 0; step < speedOf(id); ++step) {
            const Position current = positions().get(id);
            if (current == destination.target) {
                break;
            }
            const Position next{
                stepToward(current.x, destination.target.x),
                stepToward(current.y, destination.target.y),
            };
            if (!move(id, next)) {  // emits `moved` on success
                break;
            }
            moved_any = true;
            if (positions().get(id) == destination.target) {
                destination.active = false;
                marchEnded.emit(id, destination.target);
                break;
            }
        }
        return moved_any;
    }

    uint32_t speedOf(UnitId id) {
        auto& speeds = engine.components.getComponent<Speed>();
        return speeds.has(id) ? speeds.get(id).cells_per_turn : 1;
    }

    // TODO: refactor finding the type to a separate units-owner type, see: Engine.hpp TODOs
    std::optional<std::reference_wrapper<ICollisionReaction>> collisionReactionOf(UnitId id) {
        const auto type_it = engine.unit_to_type.find(id);
        if (type_it == engine.unit_to_type.end()) {
            return std::nullopt;
        }
        return type_it->second.get().findReaction<ICollisionReaction>();
    }

    bool moverIgnoresOccupants(UnitId mover_id) {
        auto reaction = collisionReactionOf(mover_id);
        return reaction && reaction->get().ignoresOccupants();
    }

    bool isMoveAllowed(UnitId occupant_id) {
        auto reaction = collisionReactionOf(occupant_id);
        return !reaction || !reaction->get().blocksMovement();
    }

    ~CorePositionSystem() override = default;
};

}  // namespace

IPositionSystemPtr MakeCorePositionSystem(Engine& engine) {
    return std::make_unique<CorePositionSystem>(engine);
}

}  // namespace sw::core
