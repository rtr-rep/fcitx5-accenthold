# fcitx5-accenthold

Hold a key to roll through its accented variants, release to type — for Linux
with [fcitx5](https://github.com/fcitx/fcitx5). Built for Omarchy / Hyprland,
works anywhere fcitx5 does.

- **Tap** `e` → `e`
- **Hold** `e` → after 0.4 s the letter in the text field rolls
  `é → è → ê → ë → e → é …` every 0.45 s
- **Release** → whatever is showing gets typed
- **Shift** (or CapsLock) + hold → capitals
- **Esc** while rolling → plain letter
- Shortcuts with Ctrl / Alt / Super are never touched

Characters go in through fcitx5 itself, so there is no fake typing, no
keyboard grab and no `input` group needed.

## Characters

| Key | Rolls through |
|-----|---------------|
| `a` | à â ä æ |
| `c` | ç |
| `e` | é è ê ë |
| `i` | î ï |
| `o` | ô ö œ |
| `s` | ß |
| `u` | ù û ü |
| `y` | ÿ |
| `<` `>` | « ‹ / » › |
| `'` | ’ ‘ |
| `"` | „ “ ” |
| `$` | € £ |

Capitals follow the same order (`S` → ẞ). Edit `variants()` in
`accenthold.cpp` to change them; `HOLD_DELAY_US` and `ROLL_STEP_US` set the
timing.

## Install

Needs `fcitx5` (with headers — Arch ships them in the main package), `cmake`,
`extra-cmake-modules` and a C++20 compiler.

```sh
./install.sh
```

This builds the addon, installs it into `~/.local` (no root) and, on Omarchy,
adds a systemd drop-in to `omarchy-fcitx5.service` setting `FCITX_ADDON_DIRS`
so fcitx5 finds it, then restarts fcitx5. Elsewhere, start fcitx5 with
`FCITX_ADDON_DIRS=$HOME/.local/lib/fcitx5:/usr/lib/fcitx5`.

Re-run `./install.sh` after a major fcitx5 upgrade.

## Uninstall

```sh
./uninstall.sh
```

## Notes

- Keys with variants are typed on release instead of press — that is how a
  tap is told apart from a hold. Holding them no longer autorepeats
  (e.g. `eeeee`), same as on macOS.
- Apps must talk to fcitx5 (GTK, Qt, terminals, Chromium/Electron with
  Wayland IME enabled). Apps that don't simply type normally.

## License

MIT
