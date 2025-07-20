#ifndef EQUIP_H
#define EQUIP_H

enum equipment_flags {
	EQF_EQUIPPED = 1,
};

enum armor_type {
	ARMOR_LIGHT,
	ARMOR_MEDIUM,
	ARMOR_HEAVY,
};

// skel
typedef struct {
	unsigned short eqw, msv;
} SEQU;

// instance
typedef struct {
	unsigned eqw;
	unsigned msv;
	unsigned rare;
	unsigned flags;
} EQU;

#endif
