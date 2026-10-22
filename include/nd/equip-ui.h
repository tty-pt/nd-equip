#ifndef ND_EQUIP_UI_H
#define ND_EQUIP_UI_H

/*
 * equip-ui.h — Extensible UI component contribution for nd-equip.
 * Provides the equipment modal and action button to nd_ui slots.
 */

#include <bud/bud.h>
#include <bud/bud_app.h>
#include <nd/ui-slot-types.h>
#include "equip-bcp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ND_EQUIP_UI_IMPL
nd_equipment_state_t g_nd_equipment = {{0}};
#endif

static inline bud_node *
nd_equip_render_modal(const void *st)
{
	(void)st;
	const char *active = nd_ui_get_active_modal();
	if (!active || strcmp(active, "equip") != 0) {
		bud_node *empty = bud_element("div");
		bud_set_attr(empty, "id", "equipment-modal");
		bud_set_attr(empty, "class", "hidden");
		return empty;
	}

	bud_node *overlay = bud_element("div");
	bud_set_attr(overlay, "id", "equipment-modal");
	bud_set_attr(overlay, "class", "fixed inset-0 z-50 flex items-center justify-center bg-black/60 p-4");

	bud_node *card = bud_element("div");
	bud_set_attr(card, "class", "bg-neutral-900 border border-neutral-700 rounded-lg p-6 max-w-sm w-full flex flex-col gap-4 shadow-xl");

	bud_node *header = bud_element("div");
	bud_set_attr(header, "class", "flex justify-between items-center border-b border-neutral-800 pb-2");
	bud_node *title = bud_element("h2");
	bud_set_attr(title, "class", "text-lg font-bold text-neutral-200");
	bud_append(title, bud_text("Equipment Doll"));
	bud_append(header, title);

	bud_node *close_btn = bud_element("button");
	bud_set_attr(close_btn, "type", "button");
	bud_set_attr(close_btn, "class", "text-neutral-400 hover:text-neutral-100 text-sm");
	bud_append(close_btn, bud_text("✕"));
	/* click handler to close modal */
	bud_append(header, close_btn);
	bud_append(card, header);

	bud_node *list = bud_tpl(
		"<div class='flex flex-col gap-2 text-sm font-mono text-neutral-200'>"
		"  <div class='flex justify-between border-b border-neutral-800 py-1'><span class='text-neutral-400'>Head:</span><span>#%u</span></div>"
		"  <div class='flex justify-between border-b border-neutral-800 py-1'><span class='text-neutral-400'>Neck:</span><span>#%u</span></div>"
		"  <div class='flex justify-between border-b border-neutral-800 py-1'><span class='text-neutral-400'>Chest:</span><span>#%u</span></div>"
		"  <div class='flex justify-between border-b border-neutral-800 py-1'><span class='text-neutral-400'>Back:</span><span>#%u</span></div>"
		"  <div class='flex justify-between border-b border-neutral-800 py-1'><span class='text-neutral-400'>Pants:</span><span>#%u</span></div>"
		"  <div class='flex justify-between border-b border-neutral-800 py-1'><span class='text-neutral-400'>Right Hand:</span><span>#%u</span></div>"
		"  <div class='flex justify-between border-b border-neutral-800 py-1'><span class='text-neutral-400'>Left Hand:</span><span>#%u</span></div>"
		"  <div class='flex justify-between border-b border-neutral-800 py-1'><span class='text-neutral-400'>Right Finger:</span><span>#%u</span></div>"
		"  <div class='flex justify-between py-1'><span class='text-neutral-400'>Left Finger:</span><span>#%u</span></div>"
		"</div>",
		g_nd_equipment.slots[ND_BP_HEAD],
		g_nd_equipment.slots[ND_BP_NECK],
		g_nd_equipment.slots[ND_BP_CHEST],
		g_nd_equipment.slots[ND_BP_BACK],
		g_nd_equipment.slots[ND_BP_PANTS],
		g_nd_equipment.slots[ND_BP_RHAND],
		g_nd_equipment.slots[ND_BP_LHAND],
		g_nd_equipment.slots[ND_BP_RFINGER],
		g_nd_equipment.slots[ND_BP_LFINGER]
	);
	bud_append(card, list);
	bud_append(overlay, card);
	return overlay;
}

static inline int
nd_equip_on_action(bud_event *ev, const char *action_id)
{
	(void)ev;
	(void)action_id;
	const char *cur = nd_ui_get_active_modal();
	if (cur && strcmp(cur, "equip") == 0) {
		nd_ui_set_active_modal("");
	} else {
		nd_ui_set_active_modal("equip");
	}
#ifdef __wasm__
	bud_app_set_state();
#endif
	return 0;
}

void nd_equip_ui_register(void);

#ifdef __cplusplus
}
#endif

#endif /* ND_EQUIP_UI_H */
