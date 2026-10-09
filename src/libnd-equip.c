/* src/libnd-equip.c — nd-equip, ported to libxylem.
 *
 * Owns equipment: what is worn where, stat requirements, rarity, the equipment
 * BCP frame, and the equip/unequip commands. Co-implements nd-attr's `effect`
 * chain (weapon damage, armour defence/dodge) and nd-fight's `fighter_wt`
 * chain (wielded weapon weight).
 *
 * Original: tty-pt/nd-equip @ 409 lines main.c, from the nd-basics
 * superproject.
 *
 * This TU XY_IMPLs effect and fighter_wt. `effect` is declared in nd-attr's
 * header, so ATTR_IMPL is defined before including it -- an XY_IMPL and an
 * XY_DECL of the same name in one TU is the XY equivalent of the old SIC_DEF
 * + SIC_DECL collision. The guard suppresses attr.h's XY_DECL(attr_stat) too,
 * which this TU CALLS, so attr_stat is re-declared manually below. fighter_wt
 * is declared in nd-fight's header, which is not included at all: nothing else
 * from it is needed here.
 *
 * on_equip / on_unequip are fired but have no implementor in-tree; the
 * dispatch finds nothing and returns 0. They stay XY_DECL-only in nd/equip.h.
 */

#include <ttypt/xy-mod.h>

#include <nd/xy.h>

#include <stdlib.h>

#include <nd/attr-types.h>

#include <nd/equip.h>

/* attr_stat is called but not implemented here. The ATTR_IMPL guard above
 * suppresses the header's XY_DECLs (to protect the effect name this TU
 * XY_IMPLs), so it is re-declared here verbatim from nd/attr.h. */
XY_DECL(unsigned, attr_stat, unsigned, ref, enum attribute, at);

#define EQT(x)		(x>>6)
#define EQL(x)		(x & 15)
#define BODYPART_ID(_c) ch_bodypart_map[(int) _c]

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

static unsigned type_equipment, bcp_equipment, equipper_hd;

static enum bodypart ch_bodypart_map[] = {
	['h'] = BP_HEAD,
	['n'] = BP_NECK,
	['c'] = BP_CHEST,
	['b'] = BP_BACK,
	['w'] = BP_RHAND,
	['l'] = BP_LFINGER,
	['r'] = BP_RFINGER,
	['g'] = BP_LEGS,
};

/* API. XY_IMPL both defines the function and emits the dispatch adapter, so
 * each name gets exactly one, with its body -- no forward declarations.
 *
 * Order matters below: XY_IMPL emits a definition, so a caller has to come
 * after its callee. mcp_equipment leads because equip/unequip call it. */

static void
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

XY_IMPL(int, on_examine, unsigned, player_ref, unsigned, ref, unsigned, type)
{
	OBJ obj;
	EQU *equ = (EQU *) &obj.data;
	if (type != type_equipment)
		return 1;
	nd_get(HD_OBJ, &obj, &ref);
	nd_printf(player_ref, "Equip: eqw %u msv %u.\n", equ->eqw, equ->msv);
	return 0;
}

static unsigned equip_effect(equipper_t equipper, unsigned eql) {
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

/* Co-implementor of nd-attr's effect chain: equipment bonuses on top of the
 * base value. nd_last() gives us whatever ran before us in this dispatch. */
XY_IMPL(long, effect, unsigned, ref, enum affect, af)
{
	equipper_t equipper;
	unsigned aux;
	OBJ obj;
	EQU *equ = (EQU *) &obj.data;
	long last;
	nd_last(&last);

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

/* Co-implementor of nd-fight's fighter_wt chain: wielded weapon weight, or
 * the predecessor's value when bare-handed. */
XY_IMPL(unsigned, fighter_wt, unsigned, ref)
{
	equipper_t equipper;
	OBJ eq;
	EQU *equ = (EQU *) &eq.data;

	nd_get(equipper_hd, equipper, &ref);
	if (equipper[BP_RHAND] == NOTHING) {
		unsigned last;
		nd_last(&last);
		return last;
	}

	nd_get(HD_OBJ, &eq, &equipper[BP_RHAND]);
	return EQT(equ->eqw);
}

static int
equip_affect(unsigned ref, EQU *equ)
{
	register unsigned msv = equ->msv,
		 eqw = equ->eqw,
		 eql = EQL(eqw),
		 eqt = EQT(eqw);

	switch (eql) {
	case BP_RHAND:
		if (attr_stat(ref, ATTR_STR) < msv)
			return 1;
		break;

	case BP_HEAD:
	case BP_LEGS:
	case BP_CHEST:

		switch (eqt) {
		case ARMOR_LIGHT:
			if (attr_stat(ref, ATTR_DEX) < msv)
				return 1;
			break;
		case ARMOR_MEDIUM:
			msv /= 2;
			if (attr_stat(ref, ATTR_STR) < msv
				|| attr_stat(ref, ATTR_DEX) < msv)
				return 1;
			break;
		case ARMOR_HEAVY:
			if (attr_stat(ref, ATTR_STR) < msv)
				return 1;
		}
	}

	return 0;
}

static int
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

	nd_printf(who_ref, "You equip %s.\n", eq.name);
	nd_put(equipper_hd, &who_ref, equipper);
	mcp_content_out(who_ref, eq_ref);
	mcp_equipment(who_ref);
	on_equip(who_ref);
	return 0;
}

static unsigned
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
	on_unequip(player_ref);
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

XY_IMPL(int, on_add, unsigned, ref, unsigned, type, uint64_t, v)
{
	OBJ obj;
	SKEL skel;
	SEQU *sequ;
	EQU *enu;

	(void) v;
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

XY_IMPL(int, on_leave, unsigned, ref, unsigned, loc_ref)
{
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

XY_IMPL(int, on_auth, unsigned, player_ref)
{
	mcp_equipment(player_ref);
	return 0;
}

static void
do_equip(int fd, int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	unsigned player_ref = fd_player(fd);
	char *name = argv[1];
	unsigned eq_ref = ematch_mine(player_ref, name);

	if (eq_ref == NOTHING)
		nd_printf(player_ref, "You are not carrying that.\n");
	else if (equip(player_ref, eq_ref)) 
		nd_printf(player_ref, "You can't equip that.\n");
}

static void
do_unequip(int fd, int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	unsigned player_ref = fd_player(fd);
	char const *name = argv[1];
	enum bodypart bp = BODYPART_ID(*name);
	unsigned eq_ref;

	if ((eq_ref = unequip(player_ref, bp)) == NOTHING) {
		nd_printf(player_ref, "You can't do that.\n");
		return;
	}
}

XY_MODULE_API void
xy_install(void)
{
	/* The original mod_install took an arg and forwarded it to mod_open,
	 * which ignored it. xy_install takes none; the open sequence is inline. */
	nd_len_reg("equipper", sizeof(equipper_t));
	equipper_hd = (unsigned)nd_open("equipper", "u", "equipper", 0);

	type_equipment = (unsigned)nd_put(HD_TYPE, NULL, "equipment");
	bcp_equipment = (unsigned)nd_put(HD_BCP, NULL, "equipment");

	action_register("equip", "👕");

	nd_register("equip", do_equip, 0);
	nd_register("unequip", do_unequip, 0);
}