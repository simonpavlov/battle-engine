#include "PositionSystem.hpp"

#include <Core/CollisionReaction.hpp>
#include <Core/Engine.hpp>
#include <algorithm>
#include <typeindex>

namespace sw::core {

namespace {

uint32_t chebyshev(Position a, Position b) {
    const uint32_t dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    const uint32_t dy = a.y > b.y ? a.y - b.y : b.y - a.y;
    return std::max(dx, dy);
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
            if (restrictsMove(other_id)) {
                blocked = true;
            }
        });
        if (blocked) {
            return false;
        }

        positions().get(unit_to_move_id) = target_position;
        return true;
    }

    std::vector<UnitId> unitsInRange(Position center, uint32_t min_range, uint32_t max_range,
                                     UnitId exclude) override {
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

    // Does the unit occupying a cell forbid others from entering it?
    bool restrictsMove(UnitId occupant_id) {
        const auto type_it = engine.unit_to_type.find(occupant_id);
        if (type_it == engine.unit_to_type.end()) {
            return false;
        }
        const auto& reactions = type_it->second.get().reactions;
        const auto reaction_it = reactions.find(std::type_index(typeid(ICollisionReaction)));
        if (reaction_it == reactions.end()) {
            return false;
        }
        auto& reaction = static_cast<ICollisionReaction&>(*reaction_it->second);
        return reaction.OnCollide() == CollideReaction::RestrictMove;
    }

    ~CorePositionSystem() override = default;
};

}

IPositionSystemPtr MakeCorePositionSystem(Engine& engine) {
    return std::make_unique<CorePositionSystem>(engine);
}

}
