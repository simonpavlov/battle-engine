# Architecture

A turn-based unit battle simulation. This document describes the design as the code actually
realises it — some systems are built, others are scaffolded and marked **(planned)**. The single
overriding goal: **new units, actions, and characteristics are added in `Features/` (plus, when
needed, a command/event in `IO/`) without ever editing `Core/`.**

## Layers

```
IO/         Fixed-format command parser + event log. Changes only to add a command or event.
Core/       The engine + standard rules vocabulary: Engine, systems, component/action/reaction
            contracts, combat resolution, tick loop. Owned & stable.
Features/   One folder (or header) per unit: its blueprint (UnitType), its actions, its
            reactions. Where everyone works.
```

(These live under `src/`. `src/main.cpp` is the **composition root**: it constructs the `Engine`,
registers systems/components/unit-types, and wires the `CommandParser`. It is not part of `Core/`.)

**Dependency direction is the load-bearing rule.** The arrows point one way only:
`Features -> Core` and `IO -> Core`. **`Core/` never `#include`s a `Feature` or `IO`.** Core
publishes *abstractions* (`Component<T>`, `IAction`, `IReaction`, `ISystem`,
`EngineCommandExecutor`); Features implement and consume them. Core internals can be reworked
freely as long as those abstractions hold, and a Feature physically cannot break Core because Core
cannot see it.

**The golden rule:** adding a unit must not touch `Core/`. If it forces a `Core/` edit, the
abstraction is wrong. The two hardest planned units are the litmus test and both must drop in as
Features only:

- **Mine:** no HP, occupies no cell, cannot be attacked, delayed area-of-effect explosion.
- **Crow:** flying, immune to melee, moves 2 cells, ranged attackers' min/max distance against it
  is reduced by 1.

## The five seams (the parallel-development payoff)

Picture many developers each adding a unit the same week. The only thing that hurts is a *shared
file they all must edit*. So every place where adding a unit could force a `Core/` edit is instead
an **extension seam** (add a file), not a **modification point** (edit a shared file).

| Where a `Core/` edit would sneak in            | The seam that removes it                                            |
| ---------------------------------------------- | ------------------------------------------------------------------- |
| A central `switch(commandName)` spawn dispatch | **`EngineCommandExecutor<io::SpawnX>` specialization** — a Feature defines its own (below) |
| A central `enum UnitType` / god factory        | No enum. A "unit type" is just a **`UnitType` blueprint** built in `Features/` |
| `Core` code naming a concrete component        | **Interface dispatch**: Core consults `IReaction` interfaces, never a concrete `Flying`/`Mine` |
| Adding a new component type to storage          | **Generic, type-keyed `ComponentsLocator`** — `Core` never enumerates component types |
| A new behavior                                  | **`IAction`** — a new class in `Features/`, dropped into the blueprint's action list |

A feature is *purely additive*: new files under `Features/`, an append-only `UnitType`
registration in the composition root, and — for a unit whose spawn parameters are new — one new
`IO/Commands/Spawn<X>` struct plus one `EngineCommandExecutor<io::SpawnX>` specialization. None is
a *shared* file two unit authors must both rewrite and reconcile, and none touch `Core`.

## Entity model — ids, blueprints, and a central component store

There is **no concrete `Unit` class**. A unit is an **id** plus a reference to its **blueprint**,
with its component data held in central per-type stores.

```cpp
using UnitId     = StrongType<std::uint32_t>;   // Core/Unit.hpp
using UnitTypeId = StrongType<std::uint32_t>;

struct UnitType {                                // the blueprint
    UnitTypeId                                        id;
    std::vector<IActionPtr>                           actions;    // ordered behavior
    std::unordered_map<std::type_index, IReactionPtr> reactions;  // capability hooks, by interface
};
```

`Engine` (`Core/Engine.hpp`) owns everything: `systems` (a `SystemsLocator`, type-keyed bag of
`ISystem`), `components` (a `ComponentsLocator`, type-keyed bag of `Component<T>`), and the
`unit_to_type` / `unit_types` maps (`UnitId -> UnitType`).

- **Components are plain data in per-type stores.** Each component type has one `Component<TData>`
  store — an `unordered_map<UnitId, TData>` with `add/del/get(UnitId)` (`Core/Components.hpp`,
  `Core/PositionComponent.cpp`). `ComponentsLocator` keys these stores by `std::type_index`, so
  **Core never enumerates component types** and a Feature adds a new characteristic by registering
  a new `Component<TData>` — zero Core edits.
- **Presence/absence encodes the rule.** A unit *has* a `Health` only if it can be hurt, a
  `Position` only if it sits on the map. No separate "mortal" or "static" flag to fall out of sync;
  the rule *is* the component.
- **Systems are authoritative.** The bytes live in the central store, but the *only* code permitted
  to mutate a given component is its owning system (`IPositionSystem` for `Position`). This is a
  discipline the type system does not yet enforce (see *Known trade-offs*).
- **(planned) `Stats`** — a string-keyed scalar component (`"strength"`, `"agility"`, ...): the one
  place a Feature adds a *characteristic* with zero `Core` change, letting a **single parameterized
  `Attack`** be data-configured, not one action class per stat. Capabilities stay typed (below).

## Capabilities — reactions queried by interface, never tags

A capability is **a reaction object on the blueprint, queried by interface** — so Core can ask
about a trait polymorphically without ever naming a concrete unit or component type. `Core`
publishes the reaction interfaces; Features implement them.

```cpp
// Core/Unit.hpp
struct IReaction { virtual ~IReaction() = default; };

// Core/CollisionReaction.hpp — the first concrete reaction
enum class CollideReaction { Ignore, RestrictMove };
struct ICollisionReaction : IReaction {
    virtual CollideReaction OnCollide() = 0;
};
```

A system looks a reaction up on the unit's `UnitType::reactions` by `type_index` and, if present,
asks it. `IPositionSystem::move` (planned completion) consults `ICollisionReaction`: a unit
returning `RestrictMove` blocks the target cell ("occupies a cell"); a flying unit simply has no
collision reaction, so it never blocks and is never blocked.

Key derivations, mapping the litmus traits to this model (no flags, no special-casing):

- **"Mortal" = has a `Health` component**; no `"mortal"` flag.
- **"Untargetable" (Mine):** target filters only consider units with `Health`, and the Mine has
  none — never selected, with no dedicated interface.
- **"Occupies a cell"** is an `ICollisionReaction` returning `RestrictMove`; Crow and Mine lack it.
- **(planned) "Melee-immune" / ranged range-reduction (Crow):** new `IReaction` interfaces
  (`IMeleeImmune`, `IRangedTargetModifier`) the targeting system queries per candidate.

**Rule for a new reaction interface:** add one only when a system must ask a trait *polymorphically*
(collision asks the occupant); mortality needs none. Keeps the set small, not one per adjective.

The four units as recipes (the litmus test, all expressible as Features):

| Unit      | Components                                  | Reactions            | Actions (priority order)                    |
| --------- | ------------------------------------------- | -------------------- | ------------------------------------------- |
| Swordsman | Position, Health, Stats{strength}           | Occupies             | Attack{strength,Melee}, Move                 |
| Hunter    | Position, Health, Stats{agility,str,range}  | Occupies             | Attack{agility,Ranged,clearAdjacency}, Attack{strength,Melee}, Move |
| Crow      | Position, Health, Stats{agility}            | Flying(MeleeImmune…) | Attack{agility,Melee}, Move                  |
| Mine      | Position, Stats{power}                       | —                    | MineTrigger (stateful)                       |

(The Crow's claw and the melee units' crushing blow are the *same* `Attack` with a different stat —
not three classes; only the Mine needs a bespoke action.)

## Behavior — an ordered action list

`IAction::tryExecute(UnitId self) -> bool` (`Core/Unit.hpp`): `true` means it ran and consumed the
turn; `false` means not applicable — fall through to the next action. Each turn a unit runs its
actions in order and stops at the first `true`, so the rules fall out of composition:

- *"attack if you can, else move"* = `Attack{…,Melee}` before `Move` in the list.
- A **static** unit simply has no move action.
- A **stateful** action (the Mine's arm → detonate-next-turn) keeps its state inside its own action
  object across ticks.

An action reaches the world through **systems injected into its constructor** by the blueprint (not
a `World` handle) — pulled from `SystemsLocator`:

```cpp
// Features/Swordsman.hpp
struct MoveAction : core::IAction {
    explicit MoveAction(core::IPositionSystem& ps) : position_system(ps) {}
    core::IPositionSystem& position_system;
    bool tryExecute(core::UnitId self) override { /* read pos, call position_system.move(...) */ }
};
inline core::UnitType MakeSwordsmanType(const core::SystemsLocator& systems) {
    core::UnitType type{ .id = kSwordsmanTypeId };
    type.actions.push_back(std::make_unique<MoveAction>(systems.getSystem<core::IPositionSystem>()));
    return type;
}
```

An action supplies its own *intent* (which stat, how much) and its own *firing preconditions* (may
I act at all). *Who is a legal target* is **not** the action's business — it asks one central
targeting service (planned), so target-eligibility lives in a single place. A single parameterized
`Attack` (planned) then covers melee and ranged for every unit; the Hunter's "can't shoot with a
unit adjacent" is an attacker-side precondition on the action, not a universal targeting rule (the
Tower shoots 2..5 without it — baking it into targeting would break the golden rule). Damage
**calculation** is the action's job (Feature); damage **application** is the system's (Core). Core
never learns that "melee uses Strength."

## Engine, systems, and combat resolution

`Engine` is the entity container and the holder of all systems. Each system owns one concern
outright — its invariants and the methods that mutate the component it governs:

- **PositionSystem** (built, partial) — single source of truth for coordinates and occupancy.
  `getPosition(id)` and `move(id, target)`; consults `ICollisionReaction` for occupancy and
  (planned) runs Chebyshev range queries. *Current state:* `move`/`getPosition` are stubbed and the
  component iteration interface is still TODO (`Core/PositionSystem.cpp`).
- **(planned) TargetingSystem** — the single home of *target eligibility*: a target must have
  `Health`; melee rejects melee-immune candidates; ranged applies per-candidate range modifiers. A
  new attack action *cannot* drop a rule, because it never writes the filter — it just asks.
- **(planned) HealthSystem** — `applyDamage(DamageRequest)` / `heal`, and end-of-tick death. Units
  without `Health` are simply unaffected.
- **(planned) RNGSystem** — `pick(candidates)` / `randomInt`. Determinism is not required.

Communication is **synchronous pull, not an event bus**: an action calls a system and the effect
lands now. The decoupling a bus would give is instead delivered by **`DamageRequest` + target-side
reaction hooks** (planned):

```cpp
enum class DamageKind { Melee, Ranged, Area };
struct DamageRequest { UnitId source, target; int amount; DamageKind kind; };
// HealthSystem::applyDamage runs the target's own damage reactions, then applies. Core never sees "Crow".
```

Systems live in `Core`; **a new unit adds none** — only a genuinely new *mechanic* adds a system, a
deliberate and rare `Core` change. (The Crow's range reduction is this same target-side pattern,
applied per candidate by targeting, so every ranged unit gets it for free.)

## Spawn dispatch & composition root

`Core` knows nothing about specific units. Spawn dispatch is a **compile-time seam**: each command
type gets an `EngineCommandExecutor<TCommand>` specialization that the Feature defines, and
`Engine::executeCommand` routes to it — no `switch`.

```cpp
// Features/Swordsman.hpp — lives in the Feature, append-only, no Core edit
template <> struct sw::core::EngineCommandExecutor<io::SpawnSwordsman> {
    void execute(core::Engine& engine, const io::SpawnSwordsman& cmd) {
        engine.components.getComponent<Position>().add(UnitId{cmd.unitId}, Position{cmd.x, cmd.y});
        engine.addUnit(feature::kSwordsmanTypeId, UnitId{cmd.unitId});
    }
};
```

The composition root (`main.cpp`, outside Core) registers systems/components and each `UnitType`,
then wires `parser.add<io::SpawnSwordsman>([&](auto c){ engine.executeCommand(c); })`. Adding a unit
= write its `UnitType`/actions/reactions in `Features/`; add an `IO/Commands/Spawn<X>` struct **iff**
its spawn parameters are new; add one `EngineCommandExecutor<io::SpawnX>` specialization and one
parser+registration line. Only new files grow, plus the append-only composition root. Zero `Core/` edits.

## Core ↔ IO events (planned)

`Core` must not include `IO`. It will define an abstract sink of domain events; `IO` implements it
over the provided `EventLog`, and the IO adapter stamps the current tick onto each logged event.

```cpp
// Core (planned): one method per event kind, the closed IO contract
class EventSink {
public:
    virtual void mapCreated(uint32_t w, uint32_t h)                            = 0;
    virtual void unitSpawned(UnitId, std::string_view type, Position)          = 0;
    virtual void marchStarted(UnitId, Position from, Position to)              = 0;
    virtual void unitMoved(UnitId, Position)                                   = 0;
    virtual void marchEnded(UnitId, Position)                                  = 0;
    virtual void unitAttacked(UnitId attacker, UnitId target, int dmg, int hp) = 0;
    virtual void unitDied(UnitId)                                              = 0;
};
```

Most new units **reuse** these events; a genuinely new *kind* adds an `IO` struct plus one
`EventSink` method — the one spot a new event touches `Core`. *Current state:* none emitted yet; the
IO `CommandParser`/`EventLog` exist but are unwired.

## Tick loop & termination

**Target loop:** commands (CREATE_MAP, SPAWN_*, MARCH) run first, logged at **tick 1**; the action
loop begins at **tick 2**. Units act in **creation order**, one action per turn. A unit reduced to
`hp <= 0` mid-tick still takes its turn this tick and is removed at end of tick (`sweep()`) —
**death is never applied mid-loop.** The run ends when `aliveCount() <= 1` (every unit counts,
HP-less ones included — a lone Mine ticks until it detonates) or when a whole tick has no effect.

**Current state** (`Engine::run`, `Core/Engine.hpp`) is partial and must converge to the above:

```cpp
void Engine::run() {
    while (true) {
        bool has_action = false;
        for (const auto& [unit_id, type] : unit_to_type)          // TODO: unordered_map is NOT
            for (const auto& action : type.get().actions)         // creation order — README requires it
                if (action->tryExecute(unit_id)) { has_action = true; break; }
        if (!has_action) break;                                   // TODO: tick counter, sweep(),
    }                                                             // death, aliveCount()<=1, events
}
```

**`Move` return contract** (drives termination, planned): returns `true` only if the unit actually
moved this turn; `false` if it has no destination, has arrived, or is blocked — so a tick of all-`false` ends the run.

## Content vs mechanic — the honest boundary

**Content** (a new unit, action, characteristic, or interaction expressible through existing systems
and reactions) → **zero `Core/` edits**, the bulk of development. A **new mechanic** (a cross-cutting
rule no existing system models) → a **new system**, a deliberate reviewed `Core` change. The claim
is not "Core never changes"; it is "*content* never forces a Core change."

## Known trade-offs

- **Central per-type component stores:** a Feature adds a characteristic by registering a
  `Component<TData>` and Core never enumerates types — but lookups go through a `type_index` map
  rather than living on the entity. We forgo contiguous SoA throughput on purpose (the brief tells
  us to ignore cache locality).
- **String-keyed `Stats` (planned):** new characteristics with no `Core` enum, bought at the cost of
  compile-time checking on stat keys. Capabilities stay typed, so they keep theirs.
- **Mutation authority is by convention, not enforced:** `Component<T>::get` hands out a mutable
  `T&`, so "only the owning system writes a given component" is a discipline the type system does
  not check.
- **System injection over a service locator:** actions get only the systems they ask for, at the
  cost of slightly more blueprint wiring.
- **Approximated termination:** a unit idling beside an unreachable/occupied target could end early.
- **Area damage ignores melee/ranged immunity (planned):** immunity is a target-*selection* rule for
  melee/ranged, not for the Mine's area blast.

These, plus the live gaps the WIP carries (the un-compiling `PositionSystem` stubs, creation-order
iteration, the missing tick/death/event machinery, and edge cases such as spawning onto an occupied
or off-map cell, `MARCH` out of bounds or to a self cell, and whether the Mine detonates after the
trigger unit has left) belong in `KNOWN_ISSUES.md` (currently a stub).
