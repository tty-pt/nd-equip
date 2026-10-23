#ifndef ND_EQUIP_BCP_H
#define ND_EQUIP_BCP_H

/*
 * equip-bcp.h — Wire protocol definitions and unpacker for nd-equip.
 * Modular channel ND_BCP_EQUIPMENT (opcode 12).
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <nd/bcp-types.h>
#include <nd/bcp-meta.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ND_BCP_EQUIPMENT 12

#define ND_BCP_MSG_EQUIP_SLOT_FIELDS(F8, F16, F32, FSTR) \
	F8(slot_id) \
	F32(item_ref) \
	FSTR(name, 48)

ND_BCP_DEFINE_SCHEMA(equip_slot, 0x12, ND_BCP_MSG_EQUIP_SLOT_FIELDS)

#define ND_EQUIP_EXT_KEY "equip"

/* Body part indices in nd_equipment_state_t.slots */
enum nd_bodypart_idx {
	ND_BP_HEAD    = 0,
	ND_BP_NECK    = 1,
	ND_BP_CHEST   = 2,
	ND_BP_BACK    = 3,
	ND_BP_PANTS   = 4,
	ND_BP_RHAND   = 5,
	ND_BP_LHAND   = 6,
	ND_BP_RFINGER = 7,
	ND_BP_LFINGER = 8,
	ND_BP_MAX     = 9,
};

typedef struct {
	uint32_t slots[ND_BP_MAX];
} nd_equipment_state_t;

/*
 * Presentation-neutral slot descriptors. `row`/`col` are an abstract
 * paper-doll layout hint (DOM grids use them, TTY tables and 3D skeleton
 * sockets ignore them and use `key`/`slot_id`). `cmd` is the server verb
 * argument that unequips the slot.
 */
typedef struct {
	uint8_t     slot_id;   /* index into nd_equipment_state_t.slots */
	const char *key;       /* stable machine id, also a skeleton socket name */
	const char *label;     /* human label */
	char        cmd;       /* unequip argument letter */
	uint8_t     row, col;  /* layout hint */
} nd_equip_slot_desc_t;

static const nd_equip_slot_desc_t nd_equip_slots[] = {
	{ ND_BP_HEAD,    "head",    "Head",         'h', 0, 2 },
	{ ND_BP_RHAND,   "rhand",   "Right Hand",   'w', 1, 0 },
	{ ND_BP_NECK,    "neck",    "Neck",         'n', 1, 2 },
	{ ND_BP_BACK,    "back",    "Back",         'b', 1, 3 },
	{ ND_BP_LHAND,   "lhand",   "Left Hand",    's', 1, 4 },
	{ ND_BP_RFINGER, "rfinger", "Right Finger", 'r', 2, 0 },
	{ ND_BP_CHEST,   "chest",   "Chest",        'c', 2, 2 },
	{ ND_BP_LFINGER, "lfinger", "Left Finger",  'l', 2, 4 },
	{ ND_BP_PANTS,   "pants",   "Pants",        'g', 3, 2 },
};
#define ND_EQUIP_SLOT_COUNT (sizeof(nd_equip_slots) / sizeof(nd_equip_slots[0]))

/* Look up a descriptor by body-part index (NULL if unknown). */
static inline const nd_equip_slot_desc_t *
nd_equip_slot_by_id(unsigned slot_id)
{
	for (size_t i = 0; i < ND_EQUIP_SLOT_COUNT; i++)
		if (nd_equip_slots[i].slot_id == slot_id)
			return &nd_equip_slots[i];
	return NULL;
}

/* Plain-text equipment listing for non-DOM viewports (TTY). Returns the
 * number of bytes written (excluding NUL). */
static inline size_t
nd_equip_format_text(const nd_equipment_state_t *eq, char *buf, size_t cap)
{
	size_t n = 0;
	if (!buf || !cap)
		return 0;
	buf[0] = '\0';
	for (size_t i = 0; i < ND_EQUIP_SLOT_COUNT && n + 1 < cap; i++) {
		const nd_equip_slot_desc_t *d = &nd_equip_slots[i];
		uint32_t ref = eq ? eq->slots[d->slot_id] : 0;
		int w;
		if (ref)
			w = snprintf(buf + n, cap - n, "%-13s #%u\n", d->label, ref);
		else
			w = snprintf(buf + n, cap - n, "%-13s -\n", d->label);
		if (w < 0)
			break;
		n += (size_t)w < cap - n ? (size_t)w : cap - n - 1;
	}
	return n;
}

/* Per-session equipment lives in nd_client_state_t's extension slot "equip"
 * (no global singleton). */

static inline int
nd_equip_bcp_unpack(uint8_t opcode, const uint8_t *payload, size_t len, void *state)
{
	(void)opcode;
	if (!payload)
		return -1;

	nd_equipment_state_t *eq = nd_client_ext((nd_client_state_t *)state,
			ND_EQUIP_EXT_KEY, sizeof(nd_equipment_state_t));

	/* 1. Try decoding via Tier 2 declarative macro schema */
	if (len >= 3 && payload[0] == ND_BCP_MAGIC_0 && payload[1] == ND_BCP_MAGIC_1) {
		nd_bcp_msg_equip_slot_t slot_msg;
		int rc = nd_bcp_unpack_msg_equip_slot(payload, len, &slot_msg);
		if (rc == 1) {
			if (slot_msg.slot_id < ND_BP_MAX) {
				if (!eq)
					return -1;
				eq->slots[slot_msg.slot_id] = slot_msg.item_ref;
				return 1;
			}
			return -1; /* decoded, but names a body part we do not have */
		}
		return rc;
	}

	/* 2. Fallback to raw stream array decoding via cursor reader */
	if (!eq)
		return -1;
	nd_bcp_reader_t r;
	nd_bcp_reader_init(&r, payload, len);
	size_t count = nd_bcp_reader_rem(&r) / sizeof(uint32_t);
	if (count > ND_BP_MAX)
		count = ND_BP_MAX;

	for (size_t i = 0; i < count; i++) {
		uint32_t ref = nd_bcp_read_u32_c(&r);
		if (eq) eq->slots[i] = ref;
	}
	return nd_bcp_reader_ok(&r) ? 1 : -1;
}

static inline int
nd_equip_bcp_pack(uint8_t *buf, size_t cap, const nd_equipment_state_t *eq)
{
	if (!buf || !eq)
		return -1;

	nd_bcp_writer_t w;
	nd_bcp_writer_init(&w, buf, cap);
	for (size_t i = 0; i < ND_BP_MAX; i++) {
		nd_bcp_write_u32(&w, eq->slots[i]);
	}
	return nd_bcp_writer_ok(&w) ? (int)nd_bcp_writer_len(&w, buf) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* ND_EQUIP_BCP_H */
