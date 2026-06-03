#!/usr/bin/env python3
"""Generate a battle scenario.

60 units, all placed on the border of a 40x40 map. Every unit marches to
the mirrored cell on the opposite edge, so the whole army funnels through the
center and collapses in the middle. All three unit types are used.

Usage:
    python3 scripts/get_commands_battle.py > commands_battle.txt
    python3 scripts/get_commands_battle.py --units 60 --size 40 -o commands_battle.txt
"""

import argparse
import sys


def gen(units: int, width: int, height: int):
    spawns: list[str] = []
    marches: list[str] = []

    # Distribute units across the four edges.
    per_edge = units // 4
    counts = [per_edge, per_edge, per_edge, units - 3 * per_edge]

    # Inner range avoids the 4 corner cells so edges never share a position.
    def slots(n: int, length: int):
        # n evenly spaced coordinates inside [1, length-2]
        usable = length - 2
        if n <= 0:
            return []
        return [1 + (i * (usable - 1)) // max(n - 1, 1) for i in range(n)]

    # Destination is the start point reflected through the map center
    # {(W-1)/2, (H-1)/2}, so every unit's straight path passes through the
    # center and the whole army collapses onto {width/2, height/2}.
    def opposite(x: int, y: int):
        return (width - 1 - x, height - 1 - y)

    edge_starts = []
    # Left edge (x=0).
    edge_starts += [(0, y) for y in slots(counts[0], height)]
    # Right edge (x=W-1).
    edge_starts += [(width - 1, y) for y in slots(counts[1], height)]
    # Top edge (y=0).
    edge_starts += [(x, 0) for x in slots(counts[2], width)]
    # Bottom edge (y=H-1).
    edge_starts += [(x, height - 1) for x in slots(counts[3], width)]

    placements = [(x, y, *opposite(x, y)) for (x, y) in edge_starts]

    # Drop any accidental duplicate spawn cells (corner overlaps if size tiny).
    seen = set()
    unique = []
    for p in placements:
        if (p[0], p[1]) in seen:
            continue
        seen.add((p[0], p[1]))
        unique.append(p)

    for uid, (x, y, tx, ty) in enumerate(unique, start=1):
        kind = uid % 3
        if kind == 0:
            # Swordsman: id x y hp strength
            spawns.append(f"SPAWN_SWORDSMAN {uid} {x} {y} 50 10")
        elif kind == 1:
            # Hunter: id x y hp agility strength range
            spawns.append(f"SPAWN_HUNTER {uid} {x} {y} 40 6 8 12")
        else:
            # Raven: id x y hp agility
            spawns.append(f"SPAWN_RAVEN {uid} {x} {y} 30 4")
        marches.append(f"MARCH {uid} {tx} {ty}")

    lines = [f"CREATE_MAP {width} {height}", ""]
    lines.extend(spawns)
    lines.append("")
    lines.extend(marches)
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--units", type=int, default=60, help="total units")
    parser.add_argument("--size", type=int, default=40, help="square map side")
    parser.add_argument("--width", type=int, help="map width (overrides --size)")
    parser.add_argument("--height", type=int, help="map height (overrides --size)")
    parser.add_argument("-o", "--output", help="output file (default stdout)")
    args = parser.parse_args()

    width = args.width if args.width else args.size
    height = args.height if args.height else args.size

    text = gen(args.units, width, height)
    if args.output:
        with open(args.output, "w") as f:
            f.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
