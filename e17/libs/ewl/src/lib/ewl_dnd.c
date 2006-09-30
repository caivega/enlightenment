#include <Ewl.h>
#include "ewl_debug.h"
#include "ewl_macros.h"
#include "ewl_private.h"
#include <Ecore_Evas.h>
#include <Ecore_X.h>

#define EWL_DND_WINDOW_ROOT 0

int EWL_CALLBACK_DND_POSITION = 0; /**< A DND position event **/
int EWL_CALLBACK_DND_ENTER = 0; /**< On enter of a widget **/
int EWL_CALLBACK_DND_LEAVE = 0; /**< On exit of a widget **/
int EWL_CALLBACK_DND_DROP = 0; /**< Drop event **/
int EWL_CALLBACK_DND_DATA = 0; /**< Data event **/

static int ewl_dragging_current = 0;
static int ewl_dnd_move_count = 0;
static Ecore_Evas *ewl_dnd_drag_canvas;
static Evas *ewl_dnd_drag_evas;
static Evas_Object *ewl_dnd_drag_image;
static Ecore_X_Window ewl_dnd_evas_win;
static Ecore_X_Window ewl_dnd_drag_win = 0;

static Ewl_Widget *ewl_dnd_widget = NULL;

static Ecore_Hash *ewl_dnd_position_hash;
static Ecore_Hash *ewl_dnd_provided_hash;
static Ecore_Hash *ewl_dnd_accepted_hash;
static int ewl_dnd_status = 0;

Ecore_Event_Handler *ewl_dnd_mouse_up_handler;
Ecore_Event_Handler *ewl_dnd_mouse_move_handler;

char *ewl_dnd_drop_types[] = { "text/uri-list", "text/plain", NULL };

static char *ewl_dnd_types_encode(const char **types);
static char * ewl_dnd_type_stpcpy(char *dst, const char *src);
static int ewl_dnd_types_encoded_contains(char *types, char *type);

static int ewl_dnd_event_mouse_up(void *data, int type, void *event);
static int ewl_dnd_event_dnd_move(void *data, int type, void *event);

/**
 * @internal
 * @return Returns TRUE if the DND system was successfully initialized,
 * FALSE otherwise
 * @brief Initialize the DND sybsystem
 */
int
ewl_dnd_init(void) 
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	EWL_CALLBACK_DND_POSITION = ewl_callback_type_add();
	EWL_CALLBACK_DND_ENTER = ewl_callback_type_add();
	EWL_CALLBACK_DND_LEAVE = ewl_callback_type_add();
	EWL_CALLBACK_DND_DROP = ewl_callback_type_add();
	EWL_CALLBACK_DND_DATA = ewl_callback_type_add();

	ewl_dnd_position_hash = ecore_hash_new(ecore_direct_hash, 
						ecore_direct_compare);
	if (!ewl_dnd_position_hash)
		DRETURN_INT(FALSE, DLEVEL_STABLE);

	ewl_dnd_provided_hash = ecore_hash_new(ecore_direct_hash, 
						ecore_direct_compare);
	if (!ewl_dnd_provided_hash) {
		ecore_hash_destroy(ewl_dnd_position_hash);
		DRETURN_INT(FALSE, DLEVEL_STABLE);
	}

	ewl_dnd_accepted_hash = ecore_hash_new(ecore_direct_hash, 
						ecore_direct_compare);
	if (!ewl_dnd_accepted_hash) {
		ecore_hash_destroy(ewl_dnd_provided_hash);
		ecore_hash_destroy(ewl_dnd_position_hash);
		DRETURN_INT(FALSE, DLEVEL_STABLE);
	}

	ewl_dragging_current = 0;
	ewl_dnd_status = 1;

	DRETURN_INT(TRUE, DLEVEL_STABLE);
}

/**
 * @internal
 * @return Returns no value.
 * @brief Shuts down the EWL DND system
 */
void
ewl_dnd_shutdown(void)
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	ecore_hash_destroy(ewl_dnd_position_hash);
	ecore_hash_destroy(ewl_dnd_provided_hash);
	ecore_hash_destroy(ewl_dnd_accepted_hash);

	DLEAVE_FUNCTION(DLEVEL_STABLE);
}

/**
 * @param w: The widget to add
 * @return Returns no value
 * @brief: Adds the given widget @a w to the position hash
 */
void
ewl_dnd_position_windows_set(Ewl_Widget *w) 
{
	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR("w", w);
	DCHECK_TYPE("w", w, EWL_WIDGET_TYPE);

	ecore_hash_set(ewl_dnd_position_hash, w, (void *)1);

	DLEAVE_FUNCTION(DLEVEL_STABLE);
}

/**
 * @param w: The widget to set provided types
 * @param types: A NULL terminated array of mimetypes widget provides for DND
 * @return Returns no value
 * @brief: Sets the mimetypes the designated widget can provide for DND
 */
void
ewl_dnd_provided_types_set(Ewl_Widget *w, const char **types)
{
	char *type;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR("w", w);
	DCHECK_TYPE("w", w, EWL_WIDGET_TYPE);

	type = ecore_hash_get(ewl_dnd_provided_hash, w);
	IF_FREE(type);

	if (types && *types) {
		type = ewl_dnd_types_encode(types);
		ecore_hash_set(ewl_dnd_provided_hash, w, type);
		ewl_object_flags_add(EWL_OBJECT(w),
				EWL_FLAG_PROPERTY_DND_SOURCE,
				EWL_FLAGS_PROPERTY_MASK);
	}
	else {
		type = ecore_hash_remove(ewl_dnd_provided_hash, w);
		IF_FREE(type);
		ewl_object_flags_remove(EWL_OBJECT(w),
				EWL_FLAG_PROPERTY_DND_SOURCE,
				EWL_FLAGS_PROPERTY_MASK);
	}

	DLEAVE_FUNCTION(DLEVEL_STABLE);
}

/**
 * @param w: The widget to test for an provided type
 * @param w: The mimetype to test for provideance on a specific widget
 * @brief: Verifies the specified widget provides the given mimetype
 */
int
ewl_dnd_provided_types_contains(Ewl_Widget *w, char *type)
{
	char *types;
	int ret = FALSE;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR_RET("w", w, FALSE);
	DCHECK_TYPE_RET("w", w, EWL_WIDGET_TYPE, FALSE);

	types = ecore_hash_get(ewl_dnd_provided_hash, w);
	if (types) ret = ewl_dnd_types_encoded_contains(types, type);

	DRETURN_INT(ret, DLEVEL_STABLE);
}


/**
 * @param w: The widget to retrieve provided types
 * @return Returns a NULL terminated array of mimetypes widget provides for DND
 * @brief: Gets the mimetypes the designated widget can provide for DND
 */
const char **
ewl_dnd_provided_types_get(Ewl_Widget *w)
{
	const char **types;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR_RET("w", w, NULL);
	DCHECK_TYPE_RET("w", w, EWL_WIDGET_TYPE, NULL);

	types = ecore_hash_get(ewl_dnd_provided_hash, w);

	DRETURN_PTR(types, DLEVEL_STABLE);
}

/**
 * @param w: The widget to set accepted types
 * @param types: A NULL terminated array of mimetypes widget accepts for DND
 * @return Returns no value
 * @brief: Sets the mimetypes the designated widget can accept for DND
 */
void
ewl_dnd_accepted_types_set(Ewl_Widget *w, const char **types)
{
	char *type;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR("w", w);
	DCHECK_TYPE("w", w, EWL_WIDGET_TYPE);

	type = ecore_hash_remove(ewl_dnd_accepted_hash, w);
	IF_FREE(type);

	if (types && *types) {
		type = ewl_dnd_types_encode(types);
		ecore_hash_set(ewl_dnd_accepted_hash, w, type);
		ewl_object_flags_add(EWL_OBJECT(w),
				EWL_FLAG_PROPERTY_DND_TARGET,
				EWL_FLAGS_PROPERTY_MASK);
		if (REALIZED(w) && !OBSCURED(w)) {
			Ewl_Embed *emb;

			emb = ewl_embed_widget_find(w);
			ewl_embed_dnd_aware_set(emb);
		}
	}
	else {
		ewl_object_flags_remove(EWL_OBJECT(w),
				EWL_FLAG_PROPERTY_DND_TARGET,
				EWL_FLAGS_PROPERTY_MASK);
		if (REALIZED(w) && !OBSCURED(w)) {
			Ewl_Embed *emb;

			emb = ewl_embed_widget_find(w);
			ewl_embed_dnd_aware_remove(emb);
		}
	}

	DLEAVE_FUNCTION(DLEVEL_STABLE);
}

/**
 * @param w: The widget to test for an accepted type
 * @param w: The mimetype to test for acceptance on a specific widget
 * @brief: Verifies the specified widget accepts the given mimetype
 */
int
ewl_dnd_accepted_types_contains(Ewl_Widget *w, char *type)
{
	char *types;
	int ret = FALSE;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR_RET("w", w, FALSE);
	DCHECK_TYPE_RET("w", w, EWL_WIDGET_TYPE, FALSE);

	types = ecore_hash_get(ewl_dnd_accepted_hash, w);
	if (types) ret = ewl_dnd_types_encoded_contains(types, type);

	DRETURN_INT(ret, DLEVEL_STABLE);
}

/**
 * @param w: The widget to retrieve accepted types
 * @return Returns a NULL terminated array of mimetypes widget accepts for DND
 * @brief: Gets the mimetypes the designated widget can accept for DND
 */
const char **
ewl_dnd_accepted_types_get(Ewl_Widget *w)
{
	const char **types;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR_RET("w", w, NULL);
	DCHECK_TYPE_RET("w", w, EWL_WIDGET_TYPE, NULL);

	types = ecore_hash_get(ewl_dnd_provided_hash, w);

	DRETURN_PTR(types, DLEVEL_STABLE);
}


/**
 * @param widget: The widget to get the types for
 * @return Returns the Ewl_Dnd_Types for the given widget
 * @brief Get the Ewl_Dnd_Types for the given widget
 */
Ewl_Dnd_Types *
ewl_dnd_types_for_widget_get(Ewl_Widget *widget)
{
	Ewl_Widget *parent = NULL;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR_RET("widget", widget, NULL);
	DCHECK_TYPE_RET("widget", widget, EWL_WIDGET_TYPE, NULL);

	/* We need to get the top-level window widget.  Note
	 * that we assume here that a widget is
	 * a) Parented, and
	 * b) It's top-level parent is a window */
	parent = widget->parent;
	while (parent && parent->parent)
		parent = parent->parent;

	/* Now check if this obj we found is a window */
	if (parent && ewl_widget_type_is(parent, "embed")) 
		DRETURN_PTR(&(EWL_EMBED(parent)->dnd_types), DLEVEL_STABLE);

	DRETURN_PTR(NULL, DLEVEL_STABLE);
}

/**
 * @param w: The widget to start dragging
 * @return Returns no value
 * @brief Tells the widget to start dragging
 */
void
ewl_dnd_drag_start(Ewl_Widget *w) 
{
	int width, height;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR("w", w);
	DCHECK_TYPE("w", w, EWL_WIDGET_TYPE);

	if (!ewl_dnd_status || ewl_dragging_current) 
		DRETURN(DLEVEL_STABLE);

	ewl_dragging_current = 1;
	ewl_dnd_widget = w;
	ewl_dnd_move_count = 0;

	ewl_dnd_mouse_up_handler = ecore_event_handler_add(
						ECORE_X_EVENT_MOUSE_BUTTON_UP,
						ewl_dnd_event_mouse_up, NULL);
	ewl_dnd_mouse_move_handler = ecore_event_handler_add(
						ECORE_X_EVENT_MOUSE_MOVE, 
						ewl_dnd_event_dnd_move, NULL);

	/* XXX This needs to be changed ot correctly go through the engine */
	ewl_dnd_drag_canvas = ecore_evas_software_x11_new(NULL, 
							EWL_DND_WINDOW_ROOT, 
							64, 64, 64, 64); 
	ewl_dnd_drag_evas = ecore_evas_get(ewl_dnd_drag_canvas);

	ecore_evas_shaped_set(ewl_dnd_drag_canvas, 1);
	ecore_evas_software_x11_direct_resize_set(ewl_dnd_drag_canvas, 1);

	ewl_dnd_evas_win = ecore_evas_software_x11_window_get(
							ewl_dnd_drag_canvas);
	ecore_x_window_resize(ewl_dnd_evas_win, 64, 64);
	ecore_evas_override_set(ewl_dnd_drag_canvas, 1);

	/* ecore_evas_software_x11_direct_resize_set(ewl_dnd_drag_evas, 1); */
	ecore_evas_ignore_events_set(ewl_dnd_drag_canvas, 1);

	/* XXX Setup a cursor (This needs to become generic) */
	ewl_dnd_drag_image = evas_object_image_add(ewl_dnd_drag_evas);
	evas_object_image_file_set(ewl_dnd_drag_image, PACKAGE_DATA_DIR 
						"/images/World.png", 0);
 	evas_object_image_fill_set(ewl_dnd_drag_image, 0, 0, 50, 50);
	evas_object_resize(ewl_dnd_drag_image, 50, 50);
	evas_object_show(ewl_dnd_drag_image);

	/* Setup the dnd event capture window */
	ecore_x_window_geometry_get(EWL_DND_WINDOW_ROOT, NULL, NULL, 
							&width,  &height);
	ewl_dnd_drag_win = ecore_x_window_input_new(EWL_DND_WINDOW_ROOT, 0, 0,
								width, height);

	/* Finally show the drag window */
	ecore_x_window_show(ewl_dnd_drag_win);

	/* Confine the pointer to our event windows */	
	ecore_x_pointer_confine_grab(ewl_dnd_drag_win);
	ecore_x_keyboard_grab(ewl_dnd_drag_win);

	ecore_x_dnd_aware_set(ewl_dnd_drag_win, 1);
	ecore_x_dnd_aware_set(ewl_dnd_evas_win, 1);
	ecore_x_mwm_borderless_set(ewl_dnd_evas_win, 1);

	/* Start the drag operation */
	ecore_x_dnd_types_set(ewl_dnd_drag_win, ewl_dnd_drop_types, 1);
	ecore_x_dnd_begin(ewl_dnd_drag_win, NULL, 0);

	DLEAVE_FUNCTION(DLEVEL_STABLE);
}

/**
 * @param type: The type to check for
 * @return Returns TRUE if the given type is supported, FALSE otherwise
 * @brief Checks if the given type @a type is supported 
 */
int
ewl_dnd_type_supported(char *type)
{
	char **check;
	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR_RET("type", type, FALSE);

	for (check = ewl_dnd_drop_types; *check; check++) {
		if (!strcmp(type, *check))
			DRETURN_INT(TRUE, DLEVEL_STABLE);
	}

	DRETURN_INT(FALSE, DLEVEL_STABLE);
}

/**
 * @return Returns no value
 * @brief Disables DND
 */
void
ewl_dnd_disable(void) 
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	ewl_dnd_status = 0;

	DLEAVE_FUNCTION(DLEVEL_STABLE);
}

/**
 * @return Returns no value
 * @brief Enables DND
 */
void
ewl_dnd_enable(void) 
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	ewl_dnd_status = 1;

	DLEAVE_FUNCTION(DLEVEL_STABLE);
}

/**
 * @return Returns the current DND status
 * @brief Retrieves the current DND status
 */
int
ewl_dnd_status_get(void) 
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	DRETURN_INT(ewl_dnd_status, DLEVEL_STABLE);
}

/**
 * @return Returns the current DND widget
 * @brief Retrieves the current DND widget
 */
Ewl_Widget *
ewl_dnd_drag_widget_get(void) 
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	DRETURN_PTR(ewl_dnd_widget, DLEVEL_STABLE);
}

/**
 * @return Returns no value.
 * @brief Clears the current DND widget
 */
void
ewl_dnd_drag_widget_clear(void) 
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	ewl_dnd_widget = NULL;

	DLEAVE_FUNCTION(DLEVEL_STABLE);
}

static int
ewl_dnd_types_encoded_contains(char *types, char *type)
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	while (*types) {
		int len;

		len = strlen(types);
		if (!(strcmp(types, type)))
			DRETURN_INT(TRUE, DLEVEL_STABLE);
		types += len + 1;
	}

	DRETURN_INT(FALSE, DLEVEL_STABLE);
}


static char *
ewl_dnd_types_encode(const char **types)
{
	char *type, *tmptype;
	int count, i = 0;
	int len = 0;

	DENTER_FUNCTION(DLEVEL_STABLE);

	/*
	 * Determine the length of all types.
	 */
	for (tmptype = (char *)types[0]; tmptype; tmptype = (char *)types[i]) {
		len += strlen(tmptype) + 1;
		i++;
	}

	type = tmptype = NEW(char, len + 1);
	count = i;
	for (i = 0; i < count; i++) {
		tmptype = ewl_dnd_type_stpcpy(tmptype, types[i]);
		tmptype++;
	}
	*tmptype = '\0';

	DRETURN_PTR(type, DLEVEL_STABLE);
}

static char *
ewl_dnd_type_stpcpy(char *dst, const char *src)
{
	while (*src) {
		*dst = *src;
		dst++;
		src++;
	}
	*dst = '\0';

	return dst;
}

static int
ewl_dnd_event_dnd_move(void *data __UNUSED__, int type __UNUSED__, 
							void *event)
{
	Ecore_X_Event_Mouse_Move *ev;

	DENTER_FUNCTION(DLEVEL_STABLE);
	DCHECK_PARAM_PTR_RET("event", event, FALSE);

	ev = event;

	if (!ewl_dnd_status) DRETURN_INT(TRUE, DLEVEL_STABLE);

	ewl_dnd_move_count++;
	if (ewl_dnd_move_count == 1) 
		ecore_evas_show(ewl_dnd_drag_canvas);

	if (ewl_dnd_drag_canvas) 
		ecore_evas_move(ewl_dnd_drag_canvas, ev->x - 15, ev->y - 15);

	DRETURN_INT(TRUE, DLEVEL_STABLE);
}

static int
ewl_dnd_event_mouse_up(void *data __UNUSED__, int type __UNUSED__, 
						void *event __UNUSED__)
{
	DENTER_FUNCTION(DLEVEL_STABLE);

	if (ewl_dnd_drag_canvas && ewl_dragging_current) {
		Ecore_List *pos;
		void *val;

		ecore_x_pointer_ungrab();
		ecore_x_keyboard_ungrab();

		ecore_event_handler_del(ewl_dnd_mouse_up_handler);
		ecore_event_handler_del(ewl_dnd_mouse_move_handler);

		ecore_evas_free(ewl_dnd_drag_canvas);
		ewl_dnd_drag_canvas = NULL;
		ecore_x_window_del(ewl_dnd_drag_win);
		ecore_x_dnd_drop();

		/* Kill all last position references so they don't get
		 * carried over to the next drag */
		pos = ecore_hash_keys(ewl_dnd_position_hash);
		ecore_list_goto_first(pos);
		while ((val = ecore_list_remove_first(pos))) {
			EWL_EMBED(val)->dnd_last_position = NULL;
			ecore_hash_remove(ewl_dnd_position_hash, val);
		}
		ecore_list_destroy(pos);

		ewl_dragging_current = 0;
		ewl_widget_dnd_reset();
	}

	DRETURN_INT(TRUE, DLEVEL_STABLE);
}

