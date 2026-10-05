# Qu33ph GBA

The original marker throwing game and the Qu33ph Olympics, for the Game Boy Advance.
Ported from the DS version (Qu33phArcadeGame/qu33phds): same rules, physics, scoring and
sounds, laid out for one 240x160 screen.

## Getting the game
Push to GitHub and the **Build Qu33ph GBA** action makes `qu33ph.gba` (download it from the
run's Artifacts). Put it on an EZ-Flash's microSD card, run it in an emulator (mGBA), or write
it to a blank flash cart with a GBxCart RW / Joey Jr.

## Controls
- **Menus:** D-pad + A, B = back
- **Match:** D-pad left/right aims, hold **A** and let go to throw (**B** cancels the charge),
  **L / R** how the marker lands (vertical, angled, flat), hold **SELECT** to look up the table,
  **START** pause (then SELECT quits to the menu)
- **High scores:** L / R or left/right change page (1 PLAYER, OLYMPICS, RECORD)

## The screen
The field fills the left side and the camera follows each throw, holds on where it landed, then
comes back for your next aim. The panel on the right has the time, score, the markers left in the
set, and a map of the whole table: marker dots, the white box is what you're looking at, the dotted
line is your aim, the red dot is the chair in sudden death.

## Saving
High scores, your name, settings and your record are kept in the cartridge's battery-backed save
memory (SRAM). Flash carts with battery save and emulators pick this up by themselves.

## What's in it
1 PLAYER, 2 PLAYER (pass the GBA), OLYMPICS (with the Special Olympics), HIGH SCORES, SETTINGS.
Not yet: the shop, coins, themes, achievements, slot machine, PlinQu33ph and the arcade.

## Files
- `source/` the game. `gba.c` is all the hardware (screen, sprites, sound, save memory).
- `source/gfx.c`, `source/snd.c` are made from the DS art and sounds by
  `python3 tools/make_gba_assets.py <qu33phds>/source/assets.c` (needs numpy, pillow, scipy).
  Only needed again if the art or sounds change.
- `tools/sim.c` is a PC stand-in for `gba.c` that saves screenshots of what the GBA would show
  (`tools/sim.sh`, then `./sim script.txt outdir`; see the top of sim.c). For checking layouts only.
