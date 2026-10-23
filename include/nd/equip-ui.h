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

static inline int
on_equip_modal_close(bud_event *ev)
{
	if (ev && ev->target) {
		const char *id = bud_get_attr(ev->target, "id");
		if (!id || strcmp(id, "equipment-modal") != 0)
			return 0;
	}
	nd_ui_set_active_modal("");
#ifdef __wasm__
	bud_app_set_state();
#endif
	return 0;
}

static inline int
on_equip_card_click(bud_event *ev)
{
	if (ev) bud_event_stop_propagation(ev);
	return 1;
}

static inline bud_node *
nd_equip_make_cell(const char *slot_name, const char *label, uint32_t item_ref)
{
	bud_node *cell = bud_element("div");
	if (!slot_name) {
		bud_set_attr(cell, "class", "w-[24px] h-[24px] p-2 box-border");
		return cell;
	}

	char class_buf[160];
	snprintf(class_buf, sizeof(class_buf),
		"btn w-[24px] h-[24px] p-2 box-border flex items-center justify-center text-xs "
		"border %s cursor-pointer select-none",
		(item_ref > 0) ? "border-[#9589c5] bg-neutral-800 text-neutral-100"
		               : "border-neutral-700/60 bg-neutral-900/60 text-neutral-500");
	bud_set_attr(cell, "class", class_buf);
	bud_set_attr(cell, "data-slot", slot_name);
	bud_set_attr(cell, "title", label);

	if (slot_name) {
		const char *letter = "h";
		if (strcmp(slot_name, "head") == 0) letter = "h";
		else if (strcmp(slot_name, "neck") == 0) letter = "n";
		else if (strcmp(slot_name, "chest") == 0) letter = "c";
		else if (strcmp(slot_name, "back") == 0) letter = "b";
		else if (strcmp(slot_name, "pants") == 0) letter = "g";
		else if (strcmp(slot_name, "rhand") == 0) letter = "w";
		else if (strcmp(slot_name, "lhand") == 0) letter = "s";
		else if (strcmp(slot_name, "rfinger") == 0) letter = "r";
		else if (strcmp(slot_name, "lfinger") == 0) letter = "l";
		char cmd_buf[32];
		snprintf(cmd_buf, sizeof(cmd_buf), "unequip %s", letter);
		bud_set_attr(cell, "data-cmd", cmd_buf);
	}

	if (item_ref > 0) {
		char ref_buf[16];
		snprintf(ref_buf, sizeof(ref_buf), "#%u", item_ref);
		bud_append(cell, bud_text(ref_buf));
	} else {
		bud_append(cell, bud_text("·"));
	}
	return cell;
}

static inline bud_node *
nd_equip_render_modal(const void *st)
{
	(void)st;
	const char *active = nd_ui_get_active_modal();
	int is_open = (active && strcmp(active, "equip") == 0);

	bud_node *overlay = bud_element("div");
	bud_set_attr(overlay, "id", "equipment-modal");
	bud_set_attr(overlay, "class", is_open
		? "modal fixed inset-0 z-50 flex items-center justify-center bg-black/60"
		: "hidden");
	bud_bind(overlay, "click", 0, on_equip_modal_close);

	bud_node *doll = bud_element("div");
	bud_set_attr(doll, "id", "equipment");
	bud_set_attr(doll, "class", "flex flex-col gap-2 p-2 border border-[#9589c5] bg-[#2c2c2c]/90 shadow-2xl");
	bud_bind(doll, "click", 1, on_equip_card_click);

	/* Row 0: [ ] [ ] [HEAD] [ ] [ ] */
	bud_node *r0 = bud_element("div"); bud_set_attr(r0, "class", "flex gap-2");
	bud_append(r0, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r0, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r0, nd_equip_make_cell("head", "Head", g_nd_equipment.slots[ND_BP_HEAD]));
	bud_append(r0, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r0, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(doll, r0);

	/* Row 1: [RHAND] [ ] [NECK] [BACK] [LHAND] */
	bud_node *r1 = bud_element("div"); bud_set_attr(r1, "class", "flex gap-2");
	bud_append(r1, nd_equip_make_cell("rhand", "Right Hand", g_nd_equipment.slots[ND_BP_RHAND]));
	bud_append(r1, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r1, nd_equip_make_cell("neck", "Neck", g_nd_equipment.slots[ND_BP_NECK]));
	bud_append(r1, nd_equip_make_cell("back", "Back", g_nd_equipment.slots[ND_BP_BACK]));
	bud_append(r1, nd_equip_make_cell("lhand", "Left Hand", g_nd_equipment.slots[ND_BP_LHAND]));
	bud_append(doll, r1);

	/* Row 2: [RFINGER] [ ] [CHEST] [ ] [LFINGER] */
	bud_node *r2 = bud_element("div"); bud_set_attr(r2, "class", "flex gap-2");
	bud_append(r2, nd_equip_make_cell("rfinger", "Right Finger", g_nd_equipment.slots[ND_BP_RFINGER]));
	bud_append(r2, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r2, nd_equip_make_cell("chest", "Chest", g_nd_equipment.slots[ND_BP_CHEST]));
	bud_append(r2, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r2, nd_equip_make_cell("lfinger", "Left Finger", g_nd_equipment.slots[ND_BP_LFINGER]));
	bud_append(doll, r2);

	/* Row 3: [ ] [ ] [PANTS] [ ] [ ] */
	bud_node *r3 = bud_element("div"); bud_set_attr(r3, "class", "flex gap-2");
	bud_append(r3, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r3, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r3, nd_equip_make_cell("pants", "Pants", g_nd_equipment.slots[ND_BP_PANTS]));
	bud_append(r3, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(r3, nd_equip_make_cell(NULL, NULL, 0));
	bud_append(doll, r3);

	bud_append(overlay, doll);
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
