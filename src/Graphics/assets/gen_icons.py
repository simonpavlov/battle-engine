#!/usr/bin/env python3
"""Generate 7 simple white-on-transparent 48x48 toolbar icons."""

from pathlib import Path

from PIL import Image, ImageDraw

SIZE = 48
WHITE = (255, 255, 255, 255)
OUT_DIR = Path(__file__).resolve().parent / "icons"


def new_canvas() -> tuple[Image.Image, ImageDraw.ImageDraw]:
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def draw_load() -> Image.Image:
    img, d = new_canvas()
    d.polygon([(8, 14), (20, 14), (24, 18), (8, 18)], fill=WHITE)
    d.rectangle([8, 18, 40, 38], fill=WHITE)
    return img


def draw_play() -> Image.Image:
    img, d = new_canvas()
    d.polygon([(16, 10), (16, 38), (38, 24)], fill=WHITE)
    return img


def draw_pause() -> Image.Image:
    img, d = new_canvas()
    d.rectangle([14, 10, 21, 38], fill=WHITE)
    d.rectangle([27, 10, 34, 38], fill=WHITE)
    return img


def draw_step() -> Image.Image:
    img, d = new_canvas()
    d.polygon([(12, 10), (12, 38), (30, 24)], fill=WHITE)
    d.rectangle([32, 10, 38, 38], fill=WHITE)
    return img


def draw_panel_units() -> Image.Image:
    img, d = new_canvas()
    for y in (13, 23, 33):
        d.ellipse([8, y - 3, 14, y + 3], fill=WHITE)
        d.rectangle([18, y - 2, 40, y + 2], fill=WHITE)
    return img


def draw_panel_properties() -> Image.Image:
    img, d = new_canvas()
    for i, y in enumerate((13, 23, 33)):
        d.rectangle([8, y - 1, 40, y + 1], fill=WHITE)
        knob_x = 14 + (i * 11)
        d.ellipse([knob_x - 4, y - 4, knob_x + 4, y + 4], fill=WHITE)
    return img


def draw_panel_log() -> Image.Image:
    img, d = new_canvas()
    d.rectangle([10, 8, 38, 40], outline=WHITE, width=2)
    for y in (15, 22, 29):
        d.rectangle([15, y - 1, 33, y + 1], fill=WHITE)
    return img


ICONS = {
    "load": draw_load,
    "play": draw_play,
    "pause": draw_pause,
    "step": draw_step,
    "panel_units": draw_panel_units,
    "panel_properties": draw_panel_properties,
    "panel_log": draw_panel_log,
}


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, draw_fn in ICONS.items():
        draw_fn().save(OUT_DIR / f"{name}.png")
        print(f"wrote {OUT_DIR / f'{name}.png'}")


if __name__ == "__main__":
    main()
