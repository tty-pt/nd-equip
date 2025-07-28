#include "./include/uapi/equip.h"

#include <stdlib.h>
#include <stdio.h>

#include <nd/nd.h>
#include <nd/attr.h>
#include <nd/fight.h>

#define EQT(x)		(x>>6)
#define EQL(x)		(x & 15)
#define BODYPART_ID(_c) ch_bodypart_map[(int) _c]

#define RARE_MAX 6

#define MSRA(ms, ra, G) G(ms) * (ra + 1) / RARE_MAX
#define IE(equ, G) MSRA(equ->msv, equ->rare, G)

#define DMG_G(v) G(v)
#define DEF_G(v) G(v)
#define DODGE_G(v) G(v)

#define DODGE_ARMOR(def) def / 4
#define DEF_ARMOR(equ, aux) (IE(equ, DEF_G) >> aux)
#define DMG_WEAPON(equ) IE(equ, DMG_G)

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

unsigned type_equipment, bcp_equipment, equipper_hd;

enum bodypart ch_bodypart_map[] = {
	['h'] = BP_HEAD,
	['n'] = BP_NECK,
	['c'] = BP_CHEST,
	['b'] = BP_BACK,
	['w'] = BP_RHAND,
	['l'] = BP_LFINGER,
	['r'] = BP_RFINGER,
	['g'] = BP_LEGS,
};

SIC_DEF(int, on_equip, unsigned, who_ref);
SIC_DEF(int, on_unequip, unsigned, who_ref);

#if 0
static const char *rarity_str[] = {
	ANSI_BOLD ANSI_FG_BLACK "Poor" ANSI_RESET,
	"",
	ANSI_BOLD "Uncommon" ANSI_RESET,
	ANSI_BOLD ANSI_FG_CYAN "Rare" ANSI_RESET,
	ANSI_BOLD ANSI_FG_GREEN "Epic" ANSI_RESET,
	ANSI_BOLD ANSI_FG_MAGENTA "Mythical" ANSI_RESET
};
#endif

void
mcp_equipment(unsigned player_ref)
{
	equipper_t equipper;

	nd_get(equipper_hd, equipper, &player_ref);

	fbcp(player_ref, sizeof(equipper), bcp_equipment, equipper);
	
	for (unsigned slot = 0; slot < BP_MAX; slot++) {
		register unsigned ref;
		ref = equipper[slot];
		if (ref && ref != NOTHING)
			fbcp_item(player_ref, ref, 0);
	}
}

int on_examine(unsigned player_ref, unsigned ref, unsigned type) {
	OBJ obj;
	EQU *equ = (EQU *) &obj.data;
	if (type != type_equipment)
		return 1;
	nd_get(HD_OBJ, &obj, &ref);
	nd_writef(player_ref, "Equip: eqw %u msv %u.\n", equ->eqw, equ->msv);
	return 0;
}

unsigned equip_effect(equipper_t equipper, unsigned eql) {
	OBJ obj;
	unsigned aux = equipper[eql], eqt;
	EQU *equ = (EQU *) &obj.data;

	if (aux == NOTHING)
		return 0;

	nd_get(HD_OBJ, &obj, &aux);
	eqt = EQT(equ->eqw);

	switch (eql) {
		case BP_HEAD:
			aux = 1;
			break;
		case BP_LEGS:
			aux = 2;
			break;
		case BP_CHEST:
			aux = 3;
			break;
	}

	switch (eqt) {
		case ARMOR_MEDIUM:
			aux *= 2;
			break;
		case ARMOR_HEAVY:
			aux *= 3;
	}

	return DEF_ARMOR(equ, aux);
}

long effect(unsigned ref, enum affect af) {
	equipper_t equipper;
	unsigned aux;
	OBJ obj;
	EQU *equ = (EQU *) &obj.data;
	long last;
	sic_last(&last);

	switch (af) {
		case AF_DMG:
			nd_get(equipper_hd, equipper, &ref);
			if (equipper[BP_RHAND] == NOTHING)
				return last;
			nd_get(HD_OBJ, &obj, &equipper[BP_RHAND]);
			return last + DMG_WEAPON(equ);
		case AF_DEF:
		case AF_DODGE:
			break;
		default:
		       return last;
	}

	nd_get(equipper_hd, equipper, &ref);

	aux = equip_effect(equipper, BP_LEGS)
		+ equip_effect(equipper, BP_CHEST)
		+ equip_effect(equipper, BP_HEAD);

	if (af == AF_DEF)
		return last + aux;

	int pd = last - DODGE_ARMOR(aux);
	return pd > 0 ? pd : 0;
}

unsigned fighter_wt(unsigned ref) {
	equipper_t equipper;
	OBJ eq;
	EQU *equ = (EQU *) &eq.data;

	nd_get(equipper_hd, equipper, &ref);
	if (equipper[BP_RHAND] == NOTHING) {
		unsigned last;
		sic_last(&last);
		return last;
	}

	nd_get(HD_OBJ, &eq, &equipper[BP_RHAND]);
	return EQT(equ->eqw);
}

int
equip_affect(unsigned ref, EQU *equ)
{
	register unsigned msv = equ->msv,
		 eqw = equ->eqw,
		 eql = EQL(eqw),
		 eqt = EQT(eqw);

	switch (eql) {
	case BP_RHAND:
		if (call_stat(ref, ATTR_STR) < msv)
			return 1;
		break;

	case BP_HEAD:
	case BP_LEGS:
	case BP_CHEST:

		switch (eqt) {
		case ARMOR_LIGHT:
			if (call_stat(ref, ATTR_DEX) < msv)
				return 1;
			break;
		case ARMOR_MEDIUM:
			msv /= 2;
			if (call_stat(ref, ATTR_STR) < msv
				|| call_stat(ref, ATTR_DEX) < msv)
				return 1;
			break;
		case ARMOR_HEAVY:
			if (call_stat(ref, ATTR_STR) < msv)
				return 1;
		}
	}

	return 0;
}

int
equip(unsigned who_ref, unsigned eq_ref)
{
	equipper_t equipper;
	OBJ eq;
	nd_get(HD_OBJ, &eq, &eq_ref);
	nd_get(equipper_hd, equipper, &who_ref);
	EQU *eeq = (EQU *) &eq.data;
	unsigned eql = EQL(eeq->eqw);

	if (!eql || equipper[eql] > 0
	    || equip_affect(who_ref, eeq))
		return 1;

	equipper[eql] = eq_ref;
	eeq->flags |= EQF_EQUIPPED;

	nd_writef(who_ref, "You equip %s.\n", eq.name);
	nd_put(equipper_hd, &who_ref, equipper);
	mcp_content_out(who_ref, eq_ref);
	mcp_equipment(who_ref);
	call_on_equip(who_ref);
	return 0;
}

unsigned
unequip(unsigned player_ref, unsigned eql)
{
	equipper_t equipper;
	unsigned eq_ref;

	nd_get(equipper_hd, equipper, &player_ref);
	eq_ref = equipper[eql];

	if (eq_ref == NOTHING)
		return NOTHING;

	OBJ eq;
	nd_get(HD_OBJ, &eq, &eq_ref);
	EQU *eeq = (EQU *) &eq.data;

	equipper[eql] = NOTHING;
	eeq->flags &= ~EQF_EQUIPPED;
	nd_put(equipper_hd, &player_ref, equipper);
	mcp_content_in(player_ref, eq_ref);
	mcp_equipment(player_ref);
	call_on_unequip(player_ref);
	return eq_ref;
}

static inline int
rarity_get(void) {
	register int r = random();
	if (r > RAND_MAX >> 1)
		return 0; // POOR
	if (r > RAND_MAX >> 2)
		return 1; // COMMON
	if (r > RAND_MAX >> 6)
		return 2; // UNCOMMON
	if (r > RAND_MAX >> 10)
		return 3; // RARE
	if (r > RAND_MAX >> 14)
		return 4; // EPIC
	return 5; // MYTHICAL
}

int on_add(unsigned ref, unsigned type, uint64_t v __attribute__((unused)))
{
	OBJ obj;
	SKEL skel;
	SEQU *sequ;
	EQU *enu;

	if (type == TYPE_ENTITY) {
		equipper_t equipper;
		for (int i = 0; i < BP_MAX; i++)
			equipper[i] = NOTHING;
		nd_put(equipper_hd, &ref, equipper);
		return 0;
	}

	if (type != type_equipment)
		return 1;

	nd_get(HD_OBJ, &obj, &ref);
	nd_get(HD_SKEL, &skel, &obj.skid);
	sequ = (SEQU *) &skel.data;
	enu = (EQU *) &obj.data;
	enu->eqw = sequ->eqw;
	enu->msv = sequ->msv;
	enu->rare = rarity_get();
	equip(obj.location, ref);

	nd_put(HD_OBJ, &ref, &obj);
	return 0;
}

int on_leave(unsigned ref, unsigned loc_ref) {
	OBJ obj, loc;

	nd_get(HD_OBJ, &obj, &ref);
	if (obj.type != type_equipment)
		return 1;

	nd_get(HD_OBJ, &loc, &loc_ref);

	// entity should be just like the rest
	// (HD_ENT) we could save getting the obj sometimes
	// FIXME
	if (loc.type != TYPE_ENTITY)
		return 1;

	if (!(loc.flags & OF_PLAYER))
		return 1;

	EQU *etmp = (EQU *) &obj.data;
	unequip(ref, EQL(etmp->eqw));
	return 0;
}

int on_auth(unsigned player_ref) {
	mcp_equipment(player_ref);
	return 0;
}

void
do_equip(int fd, int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	unsigned player_ref = fd_player(fd);
	char *name = argv[1];
	unsigned eq_ref = ematch_mine(player_ref, name);

	if (eq_ref == NOTHING)
		nd_writef(player_ref, "You are not carrying that.\n");
	else if (equip(player_ref, eq_ref)) 
		nd_writef(player_ref, "You can't equip that.\n");
}

void
do_unequip(int fd, int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	unsigned player_ref = fd_player(fd);
	char const *name = argv[1];
	enum bodypart bp = BODYPART_ID(*name);
	unsigned eq_ref;

	if ((eq_ref = unequip(player_ref, bp)) == NOTHING) {
		nd_writef(player_ref, CANTDO_MESSAGE);
		return;
	}
}

void mod_open(void *arg __attribute__((unused))) {
	nd_len_reg("equipper", sizeof(equipper_t));
	equipper_hd = nd_open("equipper", "u", "equipper", 0);

	type_equipment = nd_put(HD_TYPE, NULL, "equipment");
	bcp_equipment = nd_put(HD_BCP, NULL, "equipment");

	action_register("equip", "👕");

	nd_register("equip", do_equip, 0);
	nd_register("unequip", do_unequip, 0);

}

void mod_install(void *arg) {
	mod_open(arg);
}

