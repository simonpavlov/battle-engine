# Architecture

A turn-based unit battle simulation. This document states the design principles, not the full
code. The single overriding goal: **new units, actions, and characteristics are added in
`Features/` (and, when needed, a new command/event in `IO/`) without ever editing `Core/`.**

## Layers

```
IO/         Fixed-format command parser + event log. Changes only to add a command or event.
Core/       The engine + standard rules vocabulary: World, systems, entity/action/component
            contracts, combat resolution, tick loop. Owned & stable.
Features/   One folder per unit: its blueprint, its components, its action classes. Where
            everyone works.
```

(These live under `src/`. The provided `src/main.cpp` becomes the **composition root**: it wires
the `CommandParser` to the engine and registers the blueprints. It is not part of `Core/`.)

**Dependency direction is the load-bearing rule.** The arrows point one way only:
`Features -> Core` and `IO -> Core`. **`Core/` never `#include`s a `Feature` or `IO`.** Core
publishes *abstractions* (`IComponent`, `IAction`, capability interfaces, system APIs,
`EventSink`); Features implement and consume them. This is what makes "develop Core and Features
separately, by different teams" literally true: Core internals can be reworked freely as long as
the published abstractions hold, and a Feature physically cannot break Core because Core cannot see
it.

**The golden rule:** adding a unit must not touch `Core/`. If it forces a `Core/` edit, the
abstraction is wrong. The two hardest planned units are the litmus test and both must drop in as
Features only:

- **Mine:** no HP, occupies no cell, cannot be attacked, delayed area-of-effect explosion.
- **Crow:** flying, immune to melee, moves 2 cells, ranged attackers' min/max distance against it
  is reduced by 1.

## The five seams (the parallel-development payoff)

Picture many developers each adding a unit the same week. The only thing that hurts is a *shared
file they all must edit*. So every place where adding a unit could force a `Core/` edit is instead
an **extension seam** (add a file) rather than a **modification point** (edit a shared file). There
are exactly five, each with a fixed fix:

| Where a `Core/` edit would sneak in            | The seam that removes it                                            |
| ---------------------------------------------- | ------------------------------------------------------------------- |
| A central `switch(commandName)` spawn dispatch | **Spawn registry**: a Feature self-registers `makeCrow` (see below) |
| A central `enum UnitType` / god factory        | No enum. A "unit type" is just a **blueprint** in `Features/Crow/`  |
| `Core` code naming a concrete component        | **Interface dispatch**: Core consults `IRangedTargetModifier`, never `Flying` |
| Adding a new component type to storage         | **Generic, type-keyed storage** — `Core` never enumerates component types |
| A new behavior                                 | **`IAction`** — a new class in `Features/`, dropped into the blueprint's list |

With those five as seams a feature is *purely additive*: new files under `Features/<Unit>/`, plus
**one append-only registration line in a composition root that lives outside `Core/`** (in `main`
or `Features/Registry.cpp`), plus — for a unit whose spawn parameters are new — one new
`IO/Commands/Spawn<X>` struct. None of these is a *shared* file two unit authors must both rewrite
and reconcile: each adds distinct new files and *appends* to (never rewrites) the registry. And none
of them touch `Core`.

## Entity model — composition via a typed component bag

There is one concrete `Unit`. It is *assembled*, never subclassed. It owns a heterogeneous,
type-keyed bag of components plus an ordered list of actions:

```cpp
struct IComponent { virtual ~IComponent() = default; };  // polymorphic base for all components

class Unit {
public:
    explicit Unit(uint32_t id) : _id(id) {}
    uint32_t id() const { return _id; }

    template <class T>   T*    get();           // component, or nullptr if absent
    template <class T>   bool  has() const;
    template <class IFace>
    std::vector<IFace*>        query() const;    // every component implementing IFace (dynamic_cast)

    const std::vector<std::unique_ptr<IAction>>& actions() const { return _actions; }

    bool removalPending() const { return _remove; }
    void markForRemoval()       { _remove = true; }

private:
    uint32_t                                                        _id;
    std::unordered_map<std::type_index, std::unique_ptr<IComponent>> _components;
    std::vector<std::unique_ptr<IAction>>                           _actions;
    bool                                                            _remove = false;
    friend class UnitBuilder;   // blueprints assemble units only through the builder
};
```

- **Components are plain data.** A unit *has* a `Health` only if it can be hurt, a `Movement` only
  if it can move. **Presence/absence encodes the rule** — there is no separate "mortal" or "static"
  flag to fall out of sync (this is why string `caps` are gone; see below).
- **Storage lives on the entity; systems are authoritative.** The component bytes sit in the
  `Unit`, but the *only* code permitted to mutate a given component is its owning system
  (`PositionSystem` for `Position`, etc.). This keeps the model comprehensible (a unit is a bag of
  components + actions — easy to explain to a new developer) while interface queries stay a cheap
  local scan. We deliberately do **not** use per-type SoA stores: their only advantage is cache
  locality, which the brief tells us to ignore, and they would force a registry just to answer the
  interface queries below.
- **`Stats` is string-keyed scalars** (`"strength"`, `"agility"`, `"range"`, `"power"`, ...). This
  is the one place a Feature introduces a *new characteristic* with zero `Core` change. The string
  key is not only a cost: it lets a **single parameterized `Attack` action** be configured by data
  — `Attack{"strength", Melee}` serves the Swordsman, `Attack{"agility", Ranged, 2, range}` the
  Hunter's shot — instead of one near-duplicate action class per stat. That is the "configure, don't
  code" reuse the brief rewards. Trade-off: a typo reads as a default, not a compile error —
  mitigated by Feature-local `constexpr` key constants. Capabilities are *not* string-keyed; they
  are types (next section).

## Capabilities — typed components and interfaces, never tags

A capability is **the presence of a component, queried by interface** — so it can never disagree
with the unit's actual data or behavior. `Core` publishes the interaction interfaces; Features
implement them on their components. No code ever branches on a concrete unit or component type.

```cpp
// Core-published interaction interfaces (a component opts in by inheriting one):
struct IRangedTargetModifier { virtual void modifyRange(int& minD, int& maxD) const = 0; };
struct IMeleeImmune          { virtual ~IMeleeImmune() = default; };          // marker
struct IDamageModifier       { virtual void modify(DamageRequest&) const = 0; }; // pattern for future units
```

```cpp
// Feature-side (Features/Crow/): one component bundles "flying" behaviour, Core never learns "Crow"
struct Flying : IComponent, IRangedTargetModifier, IMeleeImmune {
    void modifyRange(int& minD, int& maxD) const override {
        if (minD > 0) --minD;
        if (maxD > 0) --maxD;
    }
};
```

Key derivations (no flags, no special-casing):

- **"Mortal" = `has<Health>()`.** The old `"mortal"` cap is deleted; mortality *is* the component.
- **"Untargetable" (Mine) falls out for free:** combat target filters only consider units with a
  `Health` component, and the Mine has none — so it is never selected, with no dedicated interface.
- **"Melee-immune" (Crow):** the melee target filter additionally rejects any candidate whose
  `query<IMeleeImmune>()` is non-empty. Ranged ignores it.
- **Ranged range reduction (Crow):** see *Combat resolution* — a per-candidate, target-side hook.
- **"Occupies a cell"** is a `Core`-published marker component `Occupies`; `PositionSystem` only
  indexes occupants. The Crow and Mine simply lack it.

The four units as component recipes (the litmus test, all expressible as Features):

| Unit      | Components                                                       | Actions (priority order)                                            |
| --------- | --------------------------------------------------------------- | ------------------------------------------------------------------- |
| Swordsman | Position, Occupies, Health, Movement{1}, Stats{strength}        | Attack{strength,Melee}, MoveToDestination                           |
| Hunter    | Position, Occupies, Health, Movement{1}, Stats{agility,str,rng} | Attack{agility,Ranged,2..range,clearAdjacency}, Attack{strength,Melee}, MoveToDestination |
| Crow      | Position, Health, Movement{2}, Flying, Stats{agility}           | Attack{agility,Melee}, MoveToDestination                            |
| Mine      | Position, Stats{power}                                          | MineTrigger (stateful)                                              |

(The Crow's "claw" and the two melee units' "crushing blow" are the *same* `Attack` class
configured with a different stat — not three classes. Only the Mine needs a bespoke action.)

**Rule for adding a new interface:** introduce one only when a `Core` system or another unit must
ask about a trait *polymorphically* during resolution. Mortality needs none (component presence);
`IRangedTargetModifier` earns one because ranged attack must ask the target. This discipline keeps
the interface set small instead of growing one per adjective.

## Behavior — an ordered action list

```cpp
class IAction {
public:
    virtual ~IAction() = default;
    // true  => this action ran and consumed the unit's turn
    // false => not applicable; fall through to the next action
    virtual bool tryExecute(Unit& self, World& world) = 0;
};
```

Each turn a unit runs its actions in order and stops at the first that returns `true`, so the rules
fall out of composition:

- *"attack if you can, else move"* = `Attack{…,Melee}` before `MoveToDestination` in the list.
- A **static** unit simply has no move action.
- A **stateful** action (the Mine's arm -> detonate-next-turn) keeps its state inside its own action
  object across ticks. (Statefulness alone wouldn't justify the action model — an ECS expresses it
  just as well with an `Armed{detonateTick}` component. The action list earns its place on the
  *ordering* requirements, not on state — see *Why not a pure ECS?*.)

Actions reach the world only through `World` and its systems (next section). An action **reads**
other units' public component data through queries, but it **never mutates another `Unit`
directly** — every effect is handed to the system that owns it, by id. An action supplies its own
*intent* (which stat, how much) and its own *firing preconditions* (may I act at all, given my
situation); but *who is a legal target* is **not** the action's private business — it asks one Core
targeting service, so the target-eligibility rules live in a single place (next section). A single
parameterized `Attack` then covers melee and ranged for every unit:

```cpp
struct Attack : IAction {                 // configured per unit in the blueprint
    std::string stat;                     // which characteristic supplies the damage
    DamageKind  kind;                     // Melee or Ranged (drives target-side eligibility, below)
    int         minD = 1, maxD = 1;       // reach (ranged max comes from Stats at build time)
    bool        requiresClearAdjacency = false;  // attacker-side firing precondition (Hunter's shot)

    bool tryExecute(Unit& self, World& world) override {
        // A *firing precondition* is the attacker's own business — it depends on the shooter's
        // situation, not on any candidate. The Hunter's swift shot is forbidden while a unit stands
        // adjacent; a Tower's shot is not. So it is opt-in config here, never baked into targeting.
        if (requiresClearAdjacency && !world.position().unitsInRange(self.id(), 1, 1).empty())
            return false;                                     // fall through to the next action

        // Target *eligibility* (a property of candidates) stays central in the targeting service:
        // Health-presence, melee-immunity, and per-candidate ranged-range modifiers. A Feature can
        // neither forget nor re-implement these — it only calls candidatesFor.
        auto candidates = world.targeting().candidatesFor(self.id(), kind, minD, maxD);
        if (candidates.empty()) return false;                 // fall through to the next action

        uint32_t target = world.rng().pick(candidates);
        int amount = self.get<Stats>()->at(stat);
        world.health().applyDamage(DamageRequest{ self.id(), target, amount, kind });
        return true;
    }
};
```

So the Swordsman carries `Attack{"strength", Melee}`; the Hunter `Attack{"agility", Ranged, 2, range,
/*clearAdjacency=*/true}` then `Attack{"strength", Melee}`; the Crow `Attack{"agility", Melee}` — no
new class per unit. The Hunter's "can't shoot with a unit adjacent" rule (README) lives on the action,
not in targeting, and deliberately so: it is **not** a universal property of ranged attacks. The Tower
(`Башня`) is also a 2..5 ranged shooter but has no such restriction — baking the rule into a
`candidatesFor(Ranged, …)` filter would impose the Hunter's rule on every shooter or force a Core edit
to exempt the Tower, breaking the golden rule. The distinction is the whole point: targeting answers
*"which candidates are legal targets"* (a target property); *"may I fire at all"* is an attacker
precondition the action owns.

Damage **calculation** (which stat, how much) is the action's job and lives in the Feature; damage
**application** (modifier hooks, clamping, death, the event) is the system's job and lives in Core.
Core never learns that "melee uses Strength."

## World, systems, and combat resolution

`World` is the entity container and the single handle an action gets. Each system owns one concern
outright — its invariants and the methods that mutate the components it governs:

- **PositionSystem** — single source of truth for coordinates and occupancy. Holds the occupancy
  index, runs spatial queries (`unitsInRange(id, minD, maxD)`, Chebyshev metric) and movement
  (`moveToward`). The index and `Position` stay in sync because — *by convention* — only this system
  writes `Position` (see *Known trade-offs* on what does and does not enforce that).
- **TargetingSystem** — the single home of *target eligibility*: which candidates a hit may legally
  land on. `candidatesFor(id, kind, minD, maxD)` runs the spatial query, then applies every
  eligibility rule in one place: a target must have `Health`; melee additionally rejects any candidate
  whose `query<IMeleeImmune>()` is non-empty; ranged applies each candidate's `IRangedTargetModifier`.
  Every such rule is a property of the *candidate*. A new attack action *cannot* drop one, because it
  never writes the filter at all — it just calls `candidatesFor`. This is the one ECS property we keep
  wholesale (see *Why not a pure ECS?*). What does **not** belong here is a *firing precondition* that
  depends on the attacker's own situation rather than on a candidate — e.g. the Hunter's "no unit
  adjacent to me" shot rule, which is not universal (the Tower lacks it). Those live on the `Attack`
  action (see *Behavior*), so targeting stays purely about target legality.
- **HealthSystem** — `applyDamage(DamageRequest)` / `heal`, and the end-of-tick `sweep()`. The one
  place death happens; units without `Health` are simply unaffected.
- **RNGSystem** — `pick(candidates)` / `randomInt`. A service, not a per-tick loop. Determinism is
  not required.

Communication is **synchronous pull, not an event bus.** Every effect in this sim is immediate, so
an action calls a system and the effect lands now. The decoupling that a bus would provide is
instead delivered by **`DamageRequest` + target-side modifier hooks**:

```cpp
enum class DamageKind { Melee, Ranged, Area };
struct DamageRequest { uint32_t source, target; int amount; DamageKind kind; };

// HealthSystem::applyDamage — runs the target's own modifiers, then applies. Core never sees "Crow".
void applyDamage(DamageRequest req) {
    Unit& tgt = _world.unit(req.target);
    for (IDamageModifier* m : tgt.query<IDamageModifier>()) m->modify(req);  // shields, resist, ...
    Health* h = tgt.get<Health>();
    if (!h) return;                                  // not mortal -> no-op
    h->hp = std::max(0, h->hp - req.amount);
    _events.unitAttacked(req.source, req.target, req.amount, h->hp);
}
```

The Crow's range reduction is the same target-side pattern, evaluated **per candidate** because the
rule is a property of the target being shot at — but it lives inside `TargetingSystem`, not in any
Feature, so every ranged unit gets it for free:

```cpp
// inside TargetingSystem::candidatesFor(shooter, Ranged, baseMin, baseMax), per candidate:
int minD = baseMin, maxD = baseMax;
for (IRangedTargetModifier* m : cand.query<IRangedTargetModifier>()) m->modifyRange(minD, maxD);
bool eligible = cand.has<Health>() && dist >= minD && dist <= maxD;
```

(Accepted trade-off: `World` exposing all systems is a service locator — an action can reach any
system. Acceptable at this scale; the alternative, threading explicit dependencies into every
action, buys little here.)

Systems live in `Core`; **a new unit adds none.** Only a genuinely new *mechanic* would add a system
— a deliberate, rare `Core` change (see *Content vs mechanic*).

## Spawn registry & composition root

`Core` knows nothing about specific units. A registry maps an `IO` spawn command to a Feature
**blueprint** (a factory that assembles components + actions via the builder):

```cpp
// Features/Registry.cpp (or main) — the composition root, OUTSIDE Core, append-only
registry.spawn<io::SpawnSwordsman>(features::makeSwordsman);
registry.spawn<io::SpawnHunter>(features::makeHunter);
registry.spawn<io::SpawnCrow>(features::makeCrow);   // new unit: one new line, no Core edit
registry.spawn<io::SpawnMine>(features::makeMine);
```

This composition root replaces the provided `src/main.cpp`: it wires the `CommandParser` to the
engine and registers the blueprints, so the parser's `add<io::SpawnX>(…)` handler routes to `makeX`
through the registry instead of a `switch`.

Adding a unit = write its blueprint + components + actions in `Features/`; add an
`IO/Commands/Spawn<X>` struct **iff** its spawn parameters are new; append one registration line.
The only files that grow are new ones plus two append-only lists (the registry, and the parser
wiring if a new command was added) — never a shared file two authors must both rewrite and merge.
Zero `Core/` edits.

## Core <-> IO events

`Core` must not include `IO`. It defines an abstract sink of domain events; `IO` implements it over
the provided `EventLog`. Systems emit through the sink as effects occur.

```cpp
// Core
class EventSink {
public:
    virtual void mapCreated(uint32_t w, uint32_t h)                                 = 0;
    virtual void unitSpawned(uint32_t id, std::string_view type, Position)          = 0;
    virtual void marchStarted(uint32_t id, Position from, Position to)              = 0;
    virtual void unitMoved(uint32_t id, Position)                                   = 0;
    virtual void marchEnded(uint32_t id, Position)                                  = 0;
    virtual void unitAttacked(uint32_t attacker, uint32_t target, int dmg, int hp)  = 0;
    virtual void unitDied(uint32_t id)                                              = 0;
    // one method per event kind; the closed IO contract
};
```

The engine holds the current tick and the `IO` adapter stamps it onto each logged event. Most new
units **reuse** these events; only a genuinely new *kind* of event adds an `IO` struct plus one
`EventSink` method — the one acknowledged spot where a new event touches `Core`.

## Tick loop & termination

Command processing (CREATE_MAP, SPAWN_*, MARCH) runs first and is logged at **tick 1**; the action
loop begins at **tick 2**. A unit reduced to `hp <= 0` mid-tick still takes its turn this tick and
is removed at end of tick — per the brief ("acts in the current turn, vanishes before the next"), so
**death is never applied mid-loop.**

```cpp
void Engine::run() {
    while (true) {
        // "only one unit left" counts every unit on the map, HP-less ones included (a lone Mine
        // still ticks until it detonates) — aliveCount() means "units", not "mortals".
        if (_world.aliveCount() <= 1) break;
        bool anyActed = false;
        for (Unit& unit : _world.unitsInCreationOrder()) {   // stable creation order; nobody spawns mid-run
            if (unit.removalPending()) continue;             // defensive: a unit that self-flagged earlier this tick
            for (auto& action : unit.actions()) {
                if (action->tryExecute(unit, _world)) { anyActed = true; break; }  // one action/turn
            }
        }
        _world.sweep();          // physically erase (hp<=0) OR action-flagged despawn (Mine) — AFTER all have acted
        if (!anyActed) break;    // a whole tick with no effect -> stable, stop
        ++_tick;
    }
}
```

**`MoveToDestination` return contract** (it drives termination): returns `true` only if the
unit actually changed position this turn. Returns `false` if it has no destination, has arrived, or
is fully blocked. So a unit idling with nothing left to do reports `false`, and a tick where every
unit reports `false` ends the simulation. On arrival the move clears the destination and emits
`MARCH_ENDED`. A flying unit steps up to its `Movement.speed` (2) cells, emitting one `UNIT_MOVED`
per cell entered.

## Content vs mechanic — the honest boundary

- **Content** — a new unit, action, characteristic, or *interaction expressible through existing
  systems and hooks* → **zero `Core/` edits.** This is the bulk of business development and stays
  frictionless.
- **New mechanic** — a genuinely new cross-cutting rule no existing system models (e.g. status
  effects ticking over time) → a **new system, a deliberate, reviewed `Core` change.** Rare, and
  correctly a `Core`-team decision.

The claim is not "Core never changes"; it is "*content* never forces a Core change" — exactly what
the brief asks.

## Why not a pure ECS?

This design *is* an ECS in all but one respect: entities are ids, components are a typed bag,
capabilities are component queries, storage is generic and type-keyed, mutation is system-owned, and
damage modifiers are data read by a system. Four of the five seams above are simply how an ECS
already works. The single deliberate deviation is **behavior** — a per-entity ordered *action list*
instead of system-major passes — and three properties of the brief drive it, each of which a pure
ECS fights:

- **Per-unit priority** ("attack if you can, else move") is local data here: the order of a
  blueprint's action list. In system-major ECS it becomes the *global* order of `AttackSystem` vs
  `MoveSystem` plus a `hasActed` tag; two unit types wanting different priorities can't be expressed
  by system order and must push the priority back into a per-entity component — i.e. back into an
  action list.
- **Creation-order, one-action-per-turn** is **entity-major** ("unit 1 fully resolves, then unit
  2"). ECS is **system-major** ("each system sweeps all matching entities"), and component stores
  aren't in creation order. Honouring the brief means wrapping an entity-major loop around the
  behavior — which is exactly this engine's tick loop.
- **The system schedule becomes an order-sensitive shared file.** Ten developers each adding a unit
  with new behavior would add a *system* and slot it into a global, semantically-ordered schedule —
  a worse merge point than per-blueprint action lists, where priority is local and nobody edits a
  shared scheduler. The brief weights exactly this parallel-development friction.

We also forgo ECS's actual payoff on purpose: contiguous SoA storage exists for cache locality and
batch throughput, which the brief explicitly tells us to ignore — so pure ECS would charge its
structural costs without paying its dividend. The one place ECS is unambiguously better — a *single*
home for targeting rules, so no Feature can re-implement or forget one — we adopt directly as
`TargetingSystem`. The takeaway is not "ECS or not"; it is **keep ECS's data model and its
one-home-per-rule discipline, replace its system-major scheduler with entity-major action lists,
because the turn semantics are entity-major.**

## Known trade-offs

- **String-keyed `Stats`:** new characteristics with no `Core` enum, bought at the cost of
  compile-time checking on stat keys. (Capabilities are typed, so they keep their checking.)
- **Mutation authority is by convention, not enforced:** "only the owning system writes a given
  component" keeps the model comprehensible (e.g. `Position` and the occupancy index can't desync),
  but `Unit::get<T>()` hands out a mutable `T*`, so the rule is a discipline the type system does not
  check. Cheap to tighten later (hand actions `const T*`, mutate only through systems); left as
  convention for now and stated rather than claimed as a guarantee.
- **`World` as service locator:** cohesion and simple action code, bought at the cost of
  least-privilege — any action can reach any system.
- **Approximated termination:** "<= 1 unit, or a whole tick with no effect." A unit idling beside an
  unreachable/occupied target could end the run early. Stated, not hidden.
- **Area damage ignores melee/ranged immunity:** the Mine's explosion (`DamageKind::Area`) hits
  every mortal unit in radius, including a Crow — immunity is a target-*selection* rule for
  melee/ranged, not for area effects. A defensible reading of an underspecified case.

These trade-offs, plus edge cases left undecided — spawning onto an occupied or off-map cell,
`MARCH` to an out-of-bounds or self cell, and whether the Mine detonates regardless of the trigger
unit having since left — are carried into the deliverable `KNOWN_ISSUES.md` (currently a stub).
