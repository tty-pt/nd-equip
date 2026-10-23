#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ttypt/xy-mod.h>
#include <bud/bud.h>
#include <bud/bud_app.h>
#include <nd/ui-slot-types.h>
#include "nd_scene.h"
#include <nd/bcp-types.h>
#include <nd/equip-bcp.h>
#include <nd/equip-ui.h>

XY_LISTENER_TAG(int, on_bcp_channel, equip, uint8_t, opcode, const uint8_t *, payload, size_t, len, void *, state)
{
	if (opcode == ND_BCP_EQUIPMENT) {
		return nd_equip_bcp_unpack(opcode, payload, len, state);
	}
	int ret = 0;
	return xy_last(&ret) == XY_OK ? ret : 0;
}

XY_LISTENER_TAG(int, on_nd_ui_action, equip, const char *, action_id)
{
	if (action_id && strcmp(action_id, "equip") == 0) {
		nd_ui_toggle_modal("equip");
		return 1;
	}
	int ret = 0;
	return xy_last(&ret) == XY_OK ? ret : 0;
}

static int
equip_contribute(int slot, void *parent, const void *state)
{
	if (slot == ND_UI_SLOT_ACTIONS)
		bud_append((bud_node *)parent, nd_ui_dock_button("👕", "Equipment", "equip"));
	else if (slot == ND_UI_SLOT_MODALS)
		bud_append((bud_node *)parent, nd_equip_render_modal(state));
	return 0;
}

static void
equip_scene_sync(void *scene, const void *state)
{
	nd_scene_t *sc = (nd_scene_t *)scene;
	const nd_client_state_t *st = (const nd_client_state_t *)state;
	const nd_equipment_state_t *eq = (const nd_equipment_state_t *)
		nd_client_ext_get(st, ND_EQUIP_EXT_KEY);
	nd_scene_entity_t *me = eq ? nd_scene_find(sc, st->me) : NULL;
	if (!me)
		return;
	for (size_t i = 0; i < ND_EQUIP_SLOT_COUNT; i++) {
		uint32_t ref = eq->slots[nd_equip_slots[i].slot_id];
		if (ref)
			nd_scene_attach(me, nd_equip_slots[i].key, ref);
	}
}

ND_UI_GLUE(equip, .contribute = equip_contribute, .scene_sync = equip_scene_sync);
