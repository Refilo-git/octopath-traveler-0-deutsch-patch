# Octopath Traveler 0 - Deutsch-Patch (source)

Source code of `Deutsch-Patch.exe`, the small Windows patcher that ships with the German translation mod
for *Octopath Traveler 0* (Steam, game version 1.0.7.0). The mod adds "Deutsch" as an additional language entry
in the game's language options; the translated data itself is in `Octopath_TravelerDE_P.pak` (not part of this repository).

## What the patcher does
- Finds `Octopath_Traveler0-Win64-Shipping.exe` (Steam registry / library folders) or lets the user pick it.
- Compares the bytes at the fixed file offsets listed below with the expected original bytes.
  If anything does not match (e.g. a different game version) it aborts and changes nothing.
- Creates a backup `Octopath_Traveler0-Win64-Shipping.exe.original`, then writes the replacement bytes in place.
- Running it again offers to restore the original bytes. A previous patcher version is detected and upgraded.
- Optionally copies `Octopath_TravelerDE_P.pak` (next to the patcher) into `...\Content\Paks`.
- No network access, no installation, no registry writes (registry is only read to locate Steam).

## Patch table (file offsets in the 1.0.7.0 exe, see `patches.h` for the exact bytes)
| # | File offset | Length | Purpose |
|---|---|---|---|
| 1 | 0x468E810 | 5 | Setter -> set_lang |
| 2 | 0x47B8102 | 16 | Optionsabgleich -> set_lang |
| 3 | 0x477BF34 | 5 | Getter -> get_real @14477c934 |
| 4 | 0x4881E1B | 5 | Getter -> get_real @14488281b |
| 5 | 0x477871F | 32 | Textur-Suffix -> DE-DE bei Deutsch |
| 6 | 0x62D2670 | 223 | Code-Cave |
| 7 | 0x328 | 4 | .text VirtualSize |

Short version: the language setter/getter are redirected to a small code cave (unused space at the end of `.text`),
which maps the extra language entry to an existing language slot for text, and selects `DE-DE` texture variants
(texture hook). The `.text` section VirtualSize is extended to cover the cave.

## Build
Any C compiler for Windows works. Example with the Zig compiler (cross-compile from any OS):

```
zig cc -target x86_64-windows-gnu -O2 -s -Wl,--subsystem,windows -o Deutsch-Patch.exe patcher.c -lcomdlg32 -lshell32 -luser32
```

Or with MSVC: `cl /O2 patcher.c user32.lib comdlg32.lib shell32.lib advapi32.lib`

## Disclaimer
Use at your own risk. Steam version only. Not affiliated with Square Enix / Acquire.
