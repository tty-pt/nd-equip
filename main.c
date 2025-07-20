#include <nd/nd.h>
#include "./include/uapi/equip.h"
#include <stdlib.h>

#define EQT(x)		(x>>6)
#define EQL(x)		(x & 15)
#define WTS_WEAPON(equ) phys_wts[EQT(equ->eqw)]
#define BODYPART_ID(_c) ch_bodypart_map[(int) _c]

unsigned type_equipment, bcp_equipment;

enum bodypart {
	BP_NULL,
	BP_HEAD,
	BP_NECK,
	BP_CHEST,
	BP_BACK,
	BP_WEAPON,
	BP_LFINGER,
	BP_RFINGER,
	BP_LEGS,
};

enum bodypart ch_bodypart_map[] = {
	[0 ... 254] = 0,
	['h'] = BP_HEAD,
	['n'] = BP_NECK,
	['c'] = BP_CHEST,
	['b'] = BP_BACK,
	['w'] = BP_WEAPON,
	['l'] = BP_LFINGER,
	['r'] = BP_RFINGER,
	['g'] = BP_LEGS,
};

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
	ENT eplayer = ent_get(player_ref);
	unsigned aux;

	fbcp(player_ref, sizeof(eplayer.equipment), bcp_equipment, eplayer.equipment);
	
	aux = eplayer.equipment[ES_HEAD];
	if (aux && aux != NOTHING)
		fbcp_item(player_ref, aux, 0);
	aux = eplayer.equipment[ES_NECK];
	if (aux && aux != NOTHING)
		fbcp_item(player_ref, aux, 0);
	aux = eplayer.equipment[ES_CHEST];
	if (aux && aux != NOTHING)
		fbcp_item(player_ref, aux, 0);
	aux = eplayer.equipment[ES_BACK];
	if (aux && aux != NOTHING)
		fbcp_item(player_ref, aux, 0);
	aux = eplayer.equipment[ES_RHAND];
	if (aux && aux != NOTHING)
		fbcp_item(player_ref, aux, 0);
	aux = eplayer.equipment[ES_LFINGER];
	if (aux && aux != NOTHING)
		fbcp_item(player_ref, aux, 0);
	aux = eplayer.equipment[ES_RFINGER];
	if (aux && aux != NOTHING)
		fbcp_item(player_ref, aux, 0);
	aux = eplayer.equipment[ES_PANTS];
	if (aux && aux != NOTHING)
		fbcp_item(player_ref, aux, 0);
}

int on_examine(unsigned player_ref, unsigned thing_ref) {
	OBJ thing;
	nd_get(HD_OBJ, &thing, &thing_ref);
	if (thing.type != type_equipment)
		return 1;
	EQU *ething = (EQU *) &thing.sp.raw;
	nd_writef(player_ref, "equipment eqw %u msv %u.\n", ething->eqw, ething->msv);
	return 0;
}

int
equip_affect(ENT *ewho, EQU *equ)
{
	register unsigned msv = equ->msv,
		 eqw = equ->eqw,
		 eql = EQL(eqw),
		 eqt = EQT(eqw);

	unsigned aux = 0;

	switch (eql) {
	case ES_RHAND:
		if (ewho->attr[ATTR_STR] < msv)
			return 1;
		EFFECT(ewho, DMG).value += DMG_WEAPON(equ);
		/* ewho->wts = WTS_WEAPON(equ); */
		break;

	case ES_HEAD:
	case ES_PANTS:
		aux = 1;
	case ES_CHEST:

		switch (eqt) {
		case ARMOR_LIGHT:
			if (ewho->attr[ATTR_DEX] < msv)
				return 1;
			aux += 2;
			break;
		case ARMOR_MEDIUM:
			msv /= 2;
			if (ewho->attr[ATTR_STR] < msv
			    || ewho->attr[ATTR_DEX] < msv)
				return 1;
			aux += 1;
			break;
		case ARMOR_HEAVY:
			if (ewho->attr[ATTR_STR] < msv)
				return 1;
		}
		aux = DEF_ARMOR(equ, aux);
		EFFECT(ewho, DEF).value += aux;
		int pd = EFFECT(ewho, DODGE).value - DODGE_ARMOR(aux);
		EFFECT(ewho, DODGE).value = pd > 0 ? pd : 0;
	}

	return 0;
}

int
equip(unsigned who_ref, unsigned eq_ref)
{
	OBJ eq;
	nd_get(HD_OBJ, &eq, &eq_ref);
	ENT ewho = ent_get(who_ref);
	EQU *eeq = (EQU *) &eq.sp.raw;
	unsigned eql = EQL(eeq->eqw);

	if (!eql || EQUIP(&ewho, eql) > 0
	    || equip_affect(&ewho, eeq))
		return 1;

	EQUIP(&ewho, eql) = eq_ref;
	eeq->flags |= EQF_EQUIPPED;

	nd_writef(who_ref, "You equip %s.\n", eq.name);
	ent_set(who_ref, &ewho);
	mcp_stats(who_ref);
	mcp_content_out(who_ref, eq_ref);
	mcp_equipment(who_ref);
	return 0;
}

unsigned
unequip(unsigned player_ref, unsigned eql)
{
	ENT eplayer = ent_get(player_ref);
	unsigned eq_ref = EQUIP(&eplayer, eql);
	unsigned eqt, aux;

	if (eq_ref == NOTHING)
		return NOTHING;

	OBJ eq;
	nd_get(HD_OBJ, &eq, &eq_ref);
	EQU *eeq = (EQU *) &eq.sp.raw;
	eqt = EQT(eeq->eqw);
	aux = 0;

	switch (eql) {
	case ES_RHAND:
		EFFECT(&eplayer, DMG).value -= DMG_WEAPON(eeq);
		break;
	case ES_PANTS:
	case ES_HEAD:
		aux = 1;
	case ES_CHEST:
		switch (eqt) {
		case ARMOR_LIGHT: aux += 2; break;
		case ARMOR_MEDIUM: aux += 1; break;
		}
		aux = DEF_ARMOR(eeq, aux);
		EFFECT(&eplayer, DEF).value -= aux;
		EFFECT(&eplayer, DODGE).value += DODGE_ARMOR(aux);
	}

	EQUIP(&eplayer, eql) = NOTHING;
	eeq->flags &= ~EQF_EQUIPPED;
	ent_set(player_ref, &eplayer);
	mcp_content_in(player_ref, eq_ref);
	mcp_stats(player_ref);
	mcp_equipment(player_ref);
	return eq_ref;
}

ENT on_birth(unsigned ent_ref, ENT eplayer) {
	int i;

	for (i = 0; i < ES_MAX; i++) {
		unsigned eq = EQUIP(&eplayer, i);

		if (eq <= 0)
			continue;

		OBJ oeq;
		nd_get(HD_OBJ, &oeq, &eq);
		EQU *eeq = (EQU *) &oeq.sp.raw;
		equip_affect(&eplayer, eeq);
	}

	return eplayer;
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

int on_add(unsigned ref, uint64_t v)
{
	OBJ obj;
	SKEL skel;
	SEQU *sequ;
	EQU *enu;

	nd_get(HD_OBJ, &obj, &ref);
	if (obj.type != type_equipment)
		return 1;

	nd_get(HD_SKEL, &skel, &obj.skid);
	sequ = (SEQU *) &skel.sp.raw;
	enu = (EQU *) &obj.sp.raw;
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

	ENT ent = ent_get(loc_ref);
	if (!(ent.flags & EF_PLAYER))
		return 1;

	EQU *etmp = (EQU *) &obj.sp.raw;
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

ENT on_attack(unsigned player_ref, ENT eplayer) {
	unsigned eq_ref = EQUIP(&eplayer, ES_RHAND);

	if (eq_ref) {
		OBJ eq;
		nd_get(HD_OBJ, &eq, &eq_ref);
		EQU *equ = (EQU *) eq.sp.raw;
		eplayer.wtst = EQT(equ->eqw);
	}

	return eplayer;
}

void mod_open(void *arg __attribute__((unused))) {
	type_equipment = nd_put(HD_TYPE, NULL, "equipment");
	bcp_equipment = nd_put(HD_BCP, NULL, "equipment");
	nd_register("equip", do_equip, 0);
	nd_register("unequip", do_unequip, 0);
	action_register("equip", "👕");
}

void mod_install(void *arg) {
	mod_open(arg);
}

