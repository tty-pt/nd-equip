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
nd_equip_make_cell(const nd_equip_slot_desc_t *d, uint32_t item_ref)
{
	bud_node *cell = bud_element("div");
	if (!d) {
		bud_set_attr(cell, "class", "w-[24px] h-[24px] p-2 box-border");
		return cell;
	}

	char class_buf[200];
	snprintf(class_buf, sizeof(class_buf),
		"btn w-[24px] h-[24px] p-2 box-border flex items-center justify-center text-xs "
		"border %s cursor-pointer select-none",
		(item_ref > 0) ? "border-[#9589c5] bg-neutral-800 text-neutral-100"
		               : "border-neutral-700/60 bg-neutral-900/60 text-neutral-500");
	bud_set_attr(cell, "class", class_buf);
	bud_set_attr(cell, "data-slot", d->key);
	bud_set_attr(cell, "title", d->label);

	char cmd_buf[32];
	snprintf(cmd_buf, sizeof(cmd_buf), "unequip %c", d->cmd);
	bud_set_attr(cell, "data-cmd", cmd_buf);

	if (item_ref > 0) {
		char ref_buf[16];
		snprintf(ref_buf, sizeof(ref_buf), "#%u", item_ref);
		bud_append(cell, bud_text(ref_buf));
	} else {
		bud_append(cell, bud_text("·"));
	}
	return cell;
}

/* Grid dimensions are derived from the descriptor table, not hard-coded. */
static inline void
nd_equip_grid_dims(unsigned *rows, unsigned *cols)
{
	unsigned r = 0, c = 0;
	for (size_t i = 0; i < ND_EQUIP_SLOT_COUNT; i++) {
		if (nd_equip_slots[i].row + 1u > r) r = nd_equip_slots[i].row + 1u;
		if (nd_equip_slots[i].col + 1u > c) c = nd_equip_slots[i].col + 1u;
	}
	*rows = r;
	*cols = c;
}

static inline bud_node *
nd_equip_render_modal(const void *st)
{
	static const uint32_t k_no_slots[ND_BP_MAX] = {0};
	const nd_equipment_state_t *eq = (const nd_equipment_state_t *)
		nd_client_ext_get((const nd_client_state_t *)st, ND_EQUIP_EXT_KEY);
	const uint32_t *slots = eq ? eq->slots : k_no_slots;
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

	unsigned rows, cols;
	nd_equip_grid_dims(&rows, &cols);
	for (unsigned r = 0; r < rows; r++) {
		bud_node *row = bud_element("div");
		bud_set_attr(row, "class", "flex gap-2");
		for (unsigned c = 0; c < cols; c++) {
			const nd_equip_slot_desc_t *d = NULL;
			for (size_t i = 0; i < ND_EQUIP_SLOT_COUNT; i++)
				if (nd_equip_slots[i].row == r && nd_equip_slots[i].col == c)
					d = &nd_equip_slots[i];
			bud_append(row, nd_equip_make_cell(d, d ? slots[d->slot_id] : 0));
		}
		bud_append(doll, row);
	}

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
