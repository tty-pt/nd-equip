/* equip.h — nd-equip's cross-module API: the equip/unequip event hooks.
 *
 * Caller-facing header. Implementers include <nd/equip-types.h>, not this header.
 */

#ifndef ND_EQUIP_H
#define ND_EQUIP_H

#include <ttypt/xy.h>
#include <nd/equip-types.h>

XY_DECL(int, on_equip, unsigned, who_ref);
XY_DECL(int, on_unequip, unsigned, who_ref);

#endif /* !ND_EQUIP_H */
