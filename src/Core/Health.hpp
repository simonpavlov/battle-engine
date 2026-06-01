#pragma once

namespace sw::core {

// A unit *can be hurt* iff it has this component (no separate "mortal" flag). Signed so damage can
// drive it to or below zero before the end-of-tick sweep removes the unit.
// TODO: add max_hp to support hiller in future
struct Health {
    // TODO: int -> uint32_t
    int hp;
};

}  // namespace sw::core
