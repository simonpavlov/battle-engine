# Architecture (condensed)

Turn-based unit battle sim. **One overriding goal: new units, actions, and characteristics are added
in `Features/` (plus, when needed, a command/event in `IO/`) without ever editing `Core/`.** This is
the contract; `ARCH.md` carries the full rationale.

## Layers & dependency rule

```
IO/        Fixed-format command parser + event log. Grows only for a new command/event.
Core/      Engine + rules vocabulary: World, systems, entity/action/component contracts,
           combat resolution, tick loop. Owned & stable.
Features/  One folder per unit: blueprint, components, actions. Where everyone works.
```

All under `src/`. Arrows point one way: `Features -> Core`, `IO -> Core`. **Core never `#include`s a
Feature or IO.** Core publishes abstractions (`IComponent`, `IAction`, capability interfaces, system
APIs, `EventSink`); Features implement them, so a Feature physically cannot break Core. `main.cpp` (the
composition root that wires the parser and registers blueprints) is outside Core.

**Golden rule:** adding a unit must not touch `Core/`. The two litmus units both drop in as Features:
- **Mine:** no HP, occupies no cell, untargetable, delayed area explosion.
- **Crow:** flying, melee-immune, moves 2, ranged min/max against it reduced by 1.

## Five seams (the parallel-dev payoff)

Every place where adding a unit could force a shared-file edit is instead an *extension seam* (add a
file), not a *modification point*:

| Would-be Core edit | Seam |
| --- | --- |
| `switch(commandName)` spawn dispatch | **Spawn registry** — a Feature self-registers `makeCrow` |
| `enum UnitType` / god factory | No enum; a unit type is a **blueprint** in `Features/Crow/` |
| Core naming a concrete component | **Interface dispatch** — Core consults `IRangedTargetModifier`, never `Flying` |
| New component type in storage | **Generic type-keyed bag** — Core never enumerates component types |
| New behavior | **`IAction`** — a new class in `Features/`, added to the blueprint list |

A feature is purely additive: new files under `Features/<Unit>/` + one append-only registry line (+ an `IO/Commands/Spawn<X>` struct iff spawn params are new) — no shared file two authors must both rewrite.

## Entity model — typed component bag

One concrete `Unit`, *assembled* never subclassed: a type-keyed bag of components + an ordered action list.

```cpp
struct IComponent { virtual ~IComponent() = default; };          // polymorphic base for query<IFace>

class Unit {                          // _components: unordered_map<type_index, unique_ptr<IComponent>>
    uint32_t _id;                     // _actions:    vector<unique_ptr<IAction>>;  bool _remove
public:
    template<class T>     T*  get();                              // component, or nullptr
    template<class T>     bool has() const;
    template<class IFace> std::vector<IFace*> query() const;      // components implementing IFace
    const std::vector<std::unique_ptr<IAction>>& actions() const;
    bool removalPending() const;  void markForRemoval();
};
```

- **Components are plain data.** A unit *has* `Health` only if mortal, `Movement` only if mobile.
  **Presence/absence encodes the rule** — no separate "mortal"/"static" flag to fall out of sync.
- **`Stats` is string-keyed scalars** (`"strength"`, `"agility"`, ...): the one place a Feature adds a
  *new characteristic* with zero Core change, and it lets one parameterized `Attack` be configured by
  data rather than a class per stat. Cost — a typo reads as a default, not a compile error (mitigated
  by Feature-local `constexpr` keys); capabilities stay *typed*, not stringly.
- Only a component's owning system mutates it — by convention (`get<T>()` is mutable), not enforcement.

## Capabilities — typed components & interfaces, never tags

A capability is the *presence of a component, queried by interface*, so it can't disagree with the unit's data — Core publishes the interfaces, Features implement them:

```cpp
struct IRangedTargetModifier { virtual void modifyRange(int& minD, int& maxD) const = 0; };
struct IMeleeImmune          { virtual ~IMeleeImmune() = default; };            // marker
struct IDamageModifier       { virtual void modify(DamageRequest&) const = 0; };  // future units

// Features/Crow/: one component bundles "flying"; Core never learns "Crow"
struct Flying : IComponent, IRangedTargetModifier, IMeleeImmune {
    void modifyRange(int& a, int& b) const override { if (a>0)--a; if (b>0)--b; }
};
```

Derivations, no flags: **mortal = `has<Health>()`**; **Mine untargetable** falls out free (target
filters require `Health`, Mine has none); **Crow melee-immune** = filter rejects `query<IMeleeImmune>()`;
**occupies a cell** = Core marker `Occupies` (Crow/Mine lack it). Add an interface *only* when a system
must ask about a trait polymorphically during resolution — not one per adjective.

The four units as recipes:

| Unit | Components | Actions (priority order) |
| --- | --- | --- |
| Swordsman | Position, Occupies, Health, Movement{1}, Stats{strength} | Attack{strength,Melee}, Move |
| Hunter | Position, Occupies, Health, Movement{1}, Stats{agility,str,rng} | Attack{agility,Ranged,2..rng,clearAdjacency}, Attack{strength,Melee}, Move |
| Crow | Position, Health, Movement{2}, Flying, Stats{agility} | Attack{agility,Melee}, Move |
| Mine | Position, Stats{power} | MineTrigger (stateful) |

## Behavior — ordered action list

`IAction::tryExecute(Unit& self, World& world)` returns `true` if it ran and consumed the unit's turn.
Each turn a unit runs its actions in order, stopping at the first `true`. Rules fall out of
composition: *"attack else move"* = `Attack` before `Move`; a **static** unit just has no move action;
a **stateful** action (Mine arm→detonate-next-turn) keeps state in its own object across ticks.

Actions read other units via `query` but **never mutate another `Unit`** — effects go through systems
by id. An action owns its *intent* and its own *firing precondition* (may I act, given my situation);
but *who is a legal target* is not its business — it asks one targeting service, so target eligibility
lives in one place. One parameterized `Attack` covers melee and ranged:

```cpp
struct Attack : IAction {
    std::string stat; DamageKind kind; int minD = 1, maxD = 1;   // configured in the blueprint
    bool requiresClearAdjacency = false;                          // attacker-side firing precondition
    bool tryExecute(Unit& self, World& w) override {
        // Firing precondition (Hunter's shot, not the Tower's): depends on *my* surroundings, so it
        // lives here, not in targeting — it is not a universal ranged rule.
        if (requiresClearAdjacency && !w.position().unitsInRange(self.id(), 1, 1).empty())
            return false;
        auto cands = w.targeting().candidatesFor(self.id(), kind, minD, maxD);  // target eligibility
        if (cands.empty()) return false;                                        // fall through
        w.health().applyDamage({ self.id(), w.rng().pick(cands),
                                 self.get<Stats>()->at(stat), kind });
        return true;
    }
};
```

Damage *calculation* (which stat) is the Feature's; *application* (modifiers, clamp, death, event) is the system's — Core never learns "melee uses Strength."

## World, systems, combat

`World` is the entity container and the single handle an action gets. Each system owns one concern:

- **PositionSystem** — coordinates + occupancy index; `unitsInRange(id,min,max)` (Chebyshev),
  `moveToward`. The only writer of `Position` (by convention) — so index and position can't desync.
- **TargetingSystem** — the *single* home of *target eligibility* (which candidates a hit may land
  on). `candidatesFor(id, kind, min, max)` runs the spatial query, then applies every candidate-side
  rule: target needs `Health`; melee rejects `IMeleeImmune`; ranged applies each candidate's
  `IRangedTargetModifier`. A new action can't forget a rule — it never writes the filter. Attacker-side
  *firing preconditions* (e.g. the Hunter's "no adjacent unit" shot, which the Tower lacks) are **not**
  here — they live on the action, since they are not universal and depend on the attacker, not a target.
- **HealthSystem** — `applyDamage` / `heal` / end-of-tick `sweep()`. The one place death happens.
- **RNGSystem** — `pick` / `randomInt`. Determinism not required.

Communication is **synchronous pull**, not a bus. Decoupling comes from `DamageRequest`
(`{source, target, amount, DamageKind}`, kind ∈ {Melee, Ranged, Area}) and target-side hooks:

```cpp
void HealthSystem::applyDamage(DamageRequest req) {           // Core never sees "Crow"
    Unit& t = _world.unit(req.target);
    for (IDamageModifier* m : t.query<IDamageModifier>()) m->modify(req);
    if (Health* h = t.get<Health>()) {                       // no Health -> no-op
        h->hp = std::max(0, h->hp - req.amount);
        _events.unitAttacked(req.source, req.target, req.amount, h->hp);
    }
}
```

A new unit adds **no system**; only a genuinely new *mechanic* does (a rare, reviewed Core change).

## Spawn registry & events

The composition root (outside Core, append-only) maps each IO spawn command to a Feature blueprint —
`registry.spawn<io::SpawnCrow>(makeCrow);` is the whole cost of wiring a new unit. Core
defines `EventSink` — one method per event kind (`mapCreated`, `unitSpawned`, `marchStarted`,
`unitMoved`, `marchEnded`, `unitAttacked`, `unitDied`); IO implements it over the provided `EventLog`,
stamping the engine's current tick. Most units reuse these events; only a new *kind* of event adds an
IO struct plus one sink method.

## Tick loop & termination

Setup commands (CREATE_MAP, SPAWN_*, MARCH) process first, logged at **tick 1**; the action loop
starts at **tick 2** (matches the reference `main.cpp`). A unit at `hp<=0` still acts this tick and is
removed at end-of-tick `sweep()` — death is never applied mid-loop.

```cpp
while (true) {
    if (world.aliveCount() <= 1) break;        // "units", incl. HP-less (a lone Mine still ticks)
    bool acted = false;
    for (Unit& u : world.unitsInCreationOrder()) {
        if (u.removalPending()) continue;
        for (auto& a : u.actions()) if (a->tryExecute(u, world)) { acted = true; break; }
    }
    world.sweep();                             // erase hp<=0 or self-flagged (Mine), after all act
    if (!acted) break;                         // a whole tick with no effect -> stable, stop
    ++tick;
}
```

`Move` returns `true` only if the unit changed cell; on arrival it clears the destination and emits `MARCH_ENDED`. A tick where every unit returns `false` ends the run.

## Why not a pure ECS?

This *is* an ECS but for one thing: behavior is a per-entity **action list**, not system-major passes.
The brief forces it — **per-unit priority** is local list order (global system order can't give two
units different priorities); **creation-order, one-action-per-turn** is entity-major while ECS is
system-major; and an ECS **system schedule is an order-sensitive shared file** (a worse merge point).
ECS's real payoff (SoA cache locality) is what the brief says to ignore; the one win we keep is a single
targeting home (`TargetingSystem`).

## Known trade-offs

- **String-keyed `Stats`:** new characteristics with no enum, at the cost of stat-key compile checks.
- **Mutation authority by convention:** `get<T>()` is mutable, so "only the owning system writes" is
  discipline, not type-enforced.
- **`World` service locator:** simple, cohesive action code vs least-privilege.
- **Approximated termination:** "<=1 unit, or a tick with no effect" can end early beside an
  unreachable/occupied target.
- **Area damage ignores melee/ranged immunity:** the Mine's blast hits a Crow — immunity is a
  melee/ranged *selection* rule, not an area one. A defensible reading.
- Open edge cases (occupied/off-map spawn, MARCH validity, Mine detonation timing) → `KNOWN_ISSUES.md`.
