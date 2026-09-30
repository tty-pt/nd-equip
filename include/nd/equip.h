/* equip.h — nd-equip's cross-module API: the equip/unequip event hooks.
 *
 * These two hooks have no implementor in-tree; nd-equip fires them and the
 * dispatch finds nothing. They are declared so the firing sites compile and so
 * a future module can implement them without a header change.
 *
 * Include this from a module TU that wants to fire or implement these hooks,
 * and NOT from nd-equip's own src/libnd-equip.c without EQUIP_IMPL: an
 * XY_IMPL and an XY_DECL of the same name in one TU collide.
 *
 * NOTE: this is a MODULE-OWNED header, not an engine one. The old location was
 * `include/uapi/equip.h`; the old `~/nd/module.mk` installed it as
 * `$(PREFIX)/include/nd/equip.h`, so `nd/` is this header's home and it is
 * installed here with `FOLDER := nd`.
 */

#ifndef ND_EQUIP_H
#define ND_EQUIP_H

#include <ttypt/xy.h>

#define RARE_MAX 6

#ifndef EQUIP_IMPL

XY_DECL(int, on_equip, unsigned, who_ref);
XY_DECL(int, on_unequip, unsigned, who_ref);

#endif /* !EQUIP_IMPL */

#endif /* !ND_EQUIP_H */