/* REQUIRES: mortal, fight */

#ifndef EQUIP_H
#define EQUIP_H

#include <nd/type.h>

#define RARE_MAX 6

/* DATA */

enum equipment_flags {
	EQF_EQUIPPED = 1,
};

enum bodypart {
	BP_NULL,
	BP_HEAD,
	BP_NECK,
	BP_CHEST,
	BP_BACK,
	BP_RHAND,
	BP_LFINGER,
	BP_RFINGER,
	BP_LEGS,
	BP_MAX,
};

enum armor_type {
	ARMOR_LIGHT,
	ARMOR_MEDIUM,
	ARMOR_HEAVY,
};

typedef struct {
	unsigned short eqw, msv;
} SEQU;

typedef struct {
	unsigned eqw;
	unsigned msv;
	unsigned rare;
	unsigned flags;
} EQU;

typedef unsigned equipper_t[BP_MAX];

/* SIC */

SIC_DECL(int, on_equip, unsigned, who_ref);
SIC_DECL(int, on_unequip, unsigned, who_ref);

#endif
