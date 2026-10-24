## [1.1.0]

- **Paper Doll UI Modal (`ux/equip_ui.c`, `<nd/equip-ui.h>`)**:
  - Implemented 4x5 paper doll modal overlay with interactive unequip triggers across all 9 body equipment slots.
  - Adopted universal `BUD_DEFINE_STATE` and `BUD_SLOT_CONTRIBUTE` for equipment UI slot registration.
- **Declarative BCP Protocol Integration (`<nd/equip-bcp.h>`)**:
  - Added declarative BCP unpacker for `ND_BCP_EQUIPMENT` frames into per-session `nd_equipment_state_t`.
  - Added canonical equipment slot descriptor table.
- **Two-Header Architecture (`<nd/equip-types.h>`)**:
  - Extracted `<nd/equip-types.h>` and removed `EQUIP_IMPL` preprocessor guards.
- **Documentation**:
  - Standardized README with man-src, art, and modder guide.

## [1.0.2]

- **The sibling `-I` lines are gone.** `CFLAGS += -I$(shell cd .. && pwd)/…`
  pointed at the axil-nd/nd sibling checkouts and only existed for a dev
  build: in CI those directories do not exist and every header comes from the
  installed packages named in `.github/workflows/ci.yml`. The build now
  resolves `<nd/…>` the way a packager sees it.
- **macOS: link with `-undefined dynamic_lookup`.** macOS `ld` rejects
  undefined symbols in a shared library, but `WARN` needs `qsyslog` — an
  engine-provided function pointer resolved at `dlopen` time (Linux allows
  this by default). `-undefined dynamic_lookup` is the Darwin equivalent, set
  as `LDFLAGS-libnd-equip-Darwin` so no other platform is affected.

## [1.0.0]

- **nd-equip is now an installable library rather than a build artifact of
  the engine.** It builds and installs exactly two files,
  `lib/libnd-equip.so` and `include/nd/equip.h`, following the same layout
  as `axil-tty` and `axil-auth`, and the same layout `nd-core` was converted
  to first. Previously `make` produced an `equip.so` named by the engine's
  `mods.load` and installed nothing. There is no `lib/nd-equip.so` symlink:
  `mods.load` names this module `libnd-equip`, the installed filename, and
  `module_load_path()` only appends `.so`.

- **The link line is libxylem plus libm.** `LDLIBS := -lxylem -lm`: `G()`
  resolves to `sqrt()`. `NEEDED` is `libxylem.so`, `libm.so.6` and
  `libc.so.6`.

- **Equip/unequip are declared in `<nd/equip.h>`.** `on_equip` /
  `on_unequip` are `XY_DECL`-only there — fired, but with no implementor
  anywhere the dispatch returns 0. This TU defines `ATTR_IMPL` (it
  `XY_IMPL`s `effect`) and manually re-declares the `attr_stat` / `mp_max` /
  `hp_max` names it calls but does not implement.

- **`stat` became `attr_stat`.** The bare name collides with libc's `stat(2)`.

- **The `equipper` table stores full structs.** `equipper_t` (9 unsigneds)
  is registered with `nd_len_reg`, so `hd_mod_open` opens the table with the
  registered value type instead of truncating to 4 bytes.

- **Dropped the `nd-mod.mk` dependency.** `nd-mod.mk` has now been deleted.
