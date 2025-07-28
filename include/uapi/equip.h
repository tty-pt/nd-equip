/* REQUIRES: mortal, fight */

#ifndef EQUIP_H
#define EQUIP_H

#include <nd/type.h>

#define RARE_MAX 6

/* SIC */

SIC_DECL(int, on_equip, unsigned, who_ref);
SIC_DECL(int, on_unequip, unsigned, who_ref);

#endif
