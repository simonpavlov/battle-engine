#include "PositionSystem.hpp"

#include <Core/CollisionReaction.hpp>
#include <Core/Destination.hpp>
#include <Core/Engine.hpp>
#include <algorithm>

namespace sw::core {

namespace {

uint32_t chebyshev(Position a, Position b) {
    const uint32_t dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    const uint32_t dy = a.y > b.y ? a.y - b.y : b.y - a.y;
    return std::max(dx, dy);
}

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

        const Position current = positions().get(id);
        if (current == destination.target) {
            destination.active = false;
            marchEnded.emit(id, current);
            return false;
        }

        const Position next{
            stepToward(current.x, destination.target.x),
            stepToward(current.y, destination.target.y),
        };
        const bool moved_this_turn = move(id, next);  // emits `moved` on success
        if (moved_this_turn && positions().get(id) == destination.target) {
            destination.active = false;
            marchEnded.emit(id, destination.target);
        }
        return moved_this_turn;
    }

    bool isMoveAllowed(UnitId occupant_id) {
        const auto type_it = engine.unit_to_type.find(occupant_id);
        if (type_it == engine.unit_to_type.end()) {
            return true;
        }
        const auto& [_, type_ref] = *type_it;
        auto reaction = type_ref.get().findReaction<ICollisionReaction>();
        if (!reaction) {
            return true;
        }
        return reaction->get().OnCollide() != CollideReaction::RestrictMove;
    }

    ~CorePositionSystem() override = default;
};

}  // namespace

IPositionSystemPtr MakeCorePositionSystem(Engine& engine) {
    return std::make_unique<CorePositionSystem>(engine);
}

}  // namespace sw::core
