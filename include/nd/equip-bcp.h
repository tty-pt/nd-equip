#ifndef ND_EQUIP_BCP_H
#define ND_EQUIP_BCP_H

/*
 * equip-bcp.h — Wire protocol definitions and unpacker for nd-equip.
 * Modular channel ND_BCP_EQUIPMENT (opcode 12).
 */

#include <stddef.h>
#include <stdint.h>
#include <nd/bcp-types.h>
#include <nd/bcp-meta.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ND_BCP_EQUIPMENT 12

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

/* Global or per-session equipment state for UI representation */
#ifdef ND_EQUIP_STORAGE
nd_equipment_state_t g_nd_equipment = {{0}};
#else
extern nd_equipment_state_t g_nd_equipment;
#endif

static inline int
nd_equip_bcp_unpack(uint8_t opcode, const uint8_t *payload, size_t len, void *state)
{
	(void)state;
	if (!payload)
		return -1;

	/* 1. Try decoding via Tier 2 declarative macro schema */
	if (len >= 3 && payload[0] == ND_BCP_MAGIC_0 && payload[1] == ND_BCP_MAGIC_1) {
		nd_bcp_msg_equip_slot_t slot_msg;
		int rc = nd_bcp_unpack_msg_equip_slot(payload, len, &slot_msg);
		if (rc == 1) {
			if (slot_msg.slot_id < ND_BP_MAX) {
				g_nd_equipment.slots[slot_msg.slot_id] = slot_msg.item_ref;
				return 1;
			}
		}
		return rc;
	}

	/* 2. Fallback to raw stream array decoding via cursor reader */
	nd_bcp_reader_t r;
	nd_bcp_reader_init(&r, payload, len);
	size_t count = nd_bcp_reader_rem(&r) / sizeof(uint32_t);
	if (count > ND_BP_MAX)
		count = ND_BP_MAX;

	for (size_t i = 0; i < count; i++) {
		g_nd_equipment.slots[i] = nd_bcp_read_u32_c(&r);
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
