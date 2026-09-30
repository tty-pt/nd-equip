# axil-nd-equip

`nd-equip` for [axil-nd](../axil-nd), ported from SIC to libxylem.

Owns equipment: the `equipper` table (one 9-slot row per entity), wield/wear
effects, and the equipment display. It co-implements nd-attr's `effect` chain
(equipment bonuses on top of the base value) and nd-fight's `fighter_wt`
chain (wielded weapon weight, or the predecessor's value when bare-handed),
both read through `nd_last()`.

## Install

```sh
make install
```

Installs:

```
lib/libnd-equip.so
include/nd/equip.h
```

There is deliberately no `lib/nd-equip.so` symlink (see `axil-nd-wts` for
why: `mods.load` names the installed filename, and the OpenBSD packing list
never lists a symlink).

Also packaged for deb, apk, rpm, brew and openbsd from a `v*` tag.

## Build from source

```sh
make
```

Needs [libxylem](https://github.com/tty-pt/libxylem) and the engine's game
API, `<nd/xy.h>`, plus `<nd/attr.h>` for the chain this module co-implements
— from checkouts beside this repo or from installed packages:

```sh
git clone https://github.com/tty-pt/nd-equip && cd nd-equip
git clone https://github.com/tty-pt/axil-nd ../axil-nd
git clone https://github.com/tty-pt/nd-attr ../axil-nd-attr
make
```

Both the checkout `-I` flags and the installed-package paths are on the
command line at once (see `Makefile`), and a missing `-I` is ignored, so the
same command works either way. CI names the deps explicitly
(`axil-nd,libxylem,nd-attr`).

## What it does

* `xy_install()` registers the `equipper` table (`equipper_t` is 9
  unsigneds, one per body slot).
* `effect` adds weapon damage and armour on top of the chain value;
  `fighter_wt` answers the wielded weapon's weight; `on_examine` shows the
  kit; `on_add` initializes the row; `on_leave` and `on_auth` cover leaving
  and login display.

## Testing

There is no `test.sh` here. Behaviour is asserted by the engine's own suite:

```sh
cd ../axil-nd
make && ./test.sh
```

## Notes from the port

* `SIC_DEF` → `XY_IMPL`, `mod_install` → `xy_install`, `call_f(...)` →
  `f(...)`. `stat` became `attr_stat`: the bare name collides with libc's
  `stat(2)`.
* This TU defines `ATTR_IMPL` (it `XY_IMPL`s `effect`) but still calls
  `attr_stat`/`mp_max`/`hp_max`, so those three are manually re-declared —
  an `XY_DECL` cannot coexist with the `XY_IMPL` in one TU.
* `on_equip` / `on_unequip` are `XY_DECL`-only in `<nd/equip.h>`: fired, but
  with no implementor anywhere the dispatch returns 0.
* `G()` → `sqrt()`, so this module links `-lm` alongside libxylem.
  `NEEDED` is `libxylem.so`, `libm.so.6` and `libc.so.6`.

## License

BSD 2-Clause, carried over from `tty-pt/nd-equip`. See `LICENSE`.
