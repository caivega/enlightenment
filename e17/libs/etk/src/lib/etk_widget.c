/** @file etk_widget.c */
#include "etk_widget.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <Edje.h>
#include "etk_theme.h"
#include "etk_main.h"
#include "etk_toplevel_widget.h"
#include "etk_container.h"
#include "etk_utils.h"
#include "etk_marshallers.h"
#include "etk_signal.h"
#include "etk_signal_callback.h"
#include "etk_dnd.h"
#include "config.h"

/**
 * @addtogroup Etk_Widget
 * @{
 */

extern Evas_List *_etk_dnd_widgets;

typedef struct _Etk_Widget_Smart_Data
{
   Etk_Widget *widget;
   Etk_Widget *swallowing_widget;
} Etk_Widget_Smart_Data;

typedef struct _Etk_Widget_Swallowed_Object
{
   Evas_Object *object;
   Etk_Bool is_widget_theme_object;
   char *swallowing_part;
} Etk_Widget_Swallowed_Object;

typedef struct _Etk_Widget_Member_Object
{
   Evas_Object *object;
   Etk_Bool visible;
} Etk_Widget_Member_Object;

enum _Etk_Widget_Signal_Id
{
   ETK_WIDGET_SHOW_SIGNAL,
   ETK_WIDGET_HIDE_SIGNAL,
   ETK_WIDGET_REALIZE_SIGNAL,
   ETK_WIDGET_UNREALIZE_SIGNAL,
   ETK_WIDGET_SIZE_REQUEST_SIGNAL,
   ETK_WIDGET_SIZE_ALLOCATE_SIGNAL,
   ETK_WIDGET_MOUSE_IN_SIGNAL,
   ETK_WIDGET_MOUSE_OUT_SIGNAL,
   ETK_WIDGET_MOUSE_MOVE_SIGNAL,
   ETK_WIDGET_MOUSE_DOWN_SIGNAL,
   ETK_WIDGET_MOUSE_UP_SIGNAL,
   ETK_WIDGET_MOUSE_CLICK_SIGNAL,
   ETK_WIDGET_MOUSE_WHEEL_SIGNAL,
   ETK_WIDGET_KEY_DOWN_SIGNAL,
   ETK_WIDGET_KEY_UP_SIGNAL,
   ETK_WIDGET_ENTER_SIGNAL,
   ETK_WIDGET_LEAVE_SIGNAL,
   ETK_WIDGET_FOCUS_SIGNAL,
   ETK_WIDGET_UNFOCUS_SIGNAL,
   ETK_WIDGET_SCROLL_SIZE_CHANGED_SIGNAL,
#if HAVE_ECORE_X
   ETK_WIDGET_DRAG_DROP_SIGNAL,
   ETK_WIDGET_DRAG_MOTION_SIGNAL,
   ETK_WIDGET_DRAG_LEAVE_SIGNAL,
   ETK_WIDGET_SELECTION_GET_SIGNAL,
#endif     
   ETK_WIDGET_NUM_SIGNALS
};

enum _Etk_Widget_Property_Id
{
   ETK_WIDGET_NAME_PROPERTY,
   ETK_WIDGET_PARENT_PROPERTY,
   ETK_WIDGET_THEME_FILE_PROPERTY,
   ETK_WIDGET_THEME_GROUP_PROPERTY,
   ETK_WIDGET_WIDTH_REQUEST_PROPERTY,
   ETK_WIDGET_HEIGHT_REQUEST_PROPERTY,
   ETK_WIDGET_VISIBLE_PROPERTY,
   ETK_WIDGET_VISIBILITY_LOCKED_PROPERTY,
   ETK_WIDGET_REPEAT_EVENTS_PROPERTY,
   ETK_WIDGET_PASS_EVENTS_PROPERTY,
   ETK_WIDGET_FOCUSABLE_PROPERTY,
   ETK_WIDGET_FOCUS_ON_PRESS_PROPERTY,
   ETK_WIDGET_CAN_PASS_FOCUS_PROPERTY
};

static void _etk_widget_constructor(Etk_Widget *widget);
static void _etk_widget_destructor(Etk_Widget *widget);
static void _etk_widget_property_set(Etk_Object *object, int property_id, Etk_Property_Value *value);
static void _etk_widget_property_get(Etk_Object *object, int property_id, Etk_Property_Value *value);

static void _etk_widget_show_handler(Etk_Widget *widget);
static void _etk_widget_enter_handler(Etk_Widget *widget);
static void _etk_widget_leave_handler(Etk_Widget *widget);
static void _etk_widget_focus_handler(Etk_Widget *widget);
static void _etk_widget_unfocus_handler(Etk_Widget *widget);
static void _etk_widget_drag_drop_handler(Etk_Widget *widget);
static void _etk_widget_drag_motion_handler(Etk_Widget *widget);
static void _etk_widget_drag_leave_handler(Etk_Widget *widget);

static void _etk_widget_mouse_in_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _etk_widget_signal_mouse_in_cb(Etk_Object *object, Etk_Event_Mouse_In_Out *event, void *data);
static void _etk_widget_mouse_out_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _etk_widget_signal_mouse_out_cb(Etk_Object *object, Etk_Event_Mouse_In_Out *event, void *data);
static void _etk_widget_mouse_move_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _etk_widget_mouse_down_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _etk_widget_signal_mouse_down_cb(Etk_Object *object, Etk_Event_Mouse_Up_Down *event, void *data);
static void _etk_widget_mouse_up_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _etk_widget_mouse_wheel_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _etk_widget_key_down_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _etk_widget_signal_key_down_cb(Etk_Object *object, Etk_Event_Key_Up_Down *event, void *data);
static void _etk_widget_key_up_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);

static void _etk_widget_toplevel_parent_set(Etk_Widget *widget, void *data);
static void _etk_widget_realize_all(Etk_Widget *widget);
static void _etk_widget_unrealize_all(Etk_Widget *widget);

static void _etk_widget_redraw_queue_recursive(Etk_Widget *widget);
Etk_Bool _etk_widget_theme_object_swallow_full(Etk_Widget *swallowing_widget, const char *part, Evas_Object *object, Etk_Bool is_widget_theme_object);
static Evas_List *_etk_widget_member_object_find(Etk_Widget *widget, Evas_Object *object);

static void _etk_widget_member_object_intercept_show_cb(void *data, Evas_Object *obj);
static void _etk_widget_member_object_intercept_hide_cb(void *data, Evas_Object *obj);

static Evas_Object *_etk_widget_smart_object_add(Evas *evas, Etk_Widget *widget);
static void _etk_widget_smart_object_del(Evas_Object *object);
static void _etk_widget_smart_object_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _etk_widget_smart_object_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _etk_widget_smart_object_show(Evas_Object *object);
static void _etk_widget_smart_object_hide(Evas_Object *object);
static void _etk_widget_smart_object_clip_set(Evas_Object *object, Evas_Object *clip);
static void _etk_widget_smart_object_clip_unset(Evas_Object *object);
static void _etk_widget_swallowed_object_free(void *data);

static Etk_Signal *_etk_widget_signals[ETK_WIDGET_NUM_SIGNALS];
static Evas_Smart *_etk_widget_smart_object_smart = NULL;
static Etk_Bool _etk_widget_propagate_event = ETK_TRUE;

/**************************
 *
 * Implementation
 *
 **************************/

/**
 * @brief Gets the type of an Etk_Widget
 * @return Returns the type on an Etk_Widget
 */
Etk_Type *etk_widget_type_get()
{
   static Etk_Type *widget_type = NULL;

   if (!widget_type)
   {
      widget_type = etk_type_new("Etk_Widget", ETK_OBJECT_TYPE, sizeof(Etk_Widget), ETK_CONSTRUCTOR(_etk_widget_constructor), ETK_DESTRUCTOR(_etk_widget_destructor));

      _etk_widget_signals[ETK_WIDGET_SHOW_SIGNAL] =          etk_signal_new("show",          widget_type, ETK_MEMBER_OFFSET(Etk_Widget, show),    etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_HIDE_SIGNAL] =          etk_signal_new("hide",          widget_type, -1,                                     etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_REALIZE_SIGNAL] =       etk_signal_new("realize",       widget_type, -1,                                     etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_UNREALIZE_SIGNAL] =     etk_signal_new("unrealize",     widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_SIZE_REQUEST_SIGNAL] =  etk_signal_new("size_request",  widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_SIZE_ALLOCATE_SIGNAL] = etk_signal_new("size_allocate", widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_MOUSE_IN_SIGNAL] =      etk_signal_new("mouse_in",      widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_MOUSE_OUT_SIGNAL] =     etk_signal_new("mouse_out",     widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_MOUSE_MOVE_SIGNAL] =    etk_signal_new("mouse_move",    widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_MOUSE_DOWN_SIGNAL] =    etk_signal_new("mouse_down",    widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_MOUSE_UP_SIGNAL] =      etk_signal_new("mouse_up",      widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_MOUSE_CLICK_SIGNAL] =   etk_signal_new("mouse_click",   widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_MOUSE_WHEEL_SIGNAL] =   etk_signal_new("mouse_wheel",   widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_KEY_DOWN_SIGNAL] =      etk_signal_new("key_down",      widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_KEY_UP_SIGNAL] =        etk_signal_new("key_up",        widget_type, -1,                                     etk_marshaller_VOID__POINTER, NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_ENTER_SIGNAL] =         etk_signal_new("enter",         widget_type, ETK_MEMBER_OFFSET(Etk_Widget, enter),   etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_LEAVE_SIGNAL] =         etk_signal_new("leave",         widget_type, ETK_MEMBER_OFFSET(Etk_Widget, leave),   etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_FOCUS_SIGNAL] =         etk_signal_new("focus",         widget_type, ETK_MEMBER_OFFSET(Etk_Widget, focus),   etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_UNFOCUS_SIGNAL] =       etk_signal_new("unfocus",       widget_type, ETK_MEMBER_OFFSET(Etk_Widget, unfocus), etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_SCROLL_SIZE_CHANGED_SIGNAL] = etk_signal_new("scroll_size_changed", widget_type, -1, etk_marshaller_VOID__VOID, NULL, NULL);
#if HAVE_ECORE_X      
      _etk_widget_signals[ETK_WIDGET_DRAG_DROP_SIGNAL] =      etk_signal_new("drag_drop",      widget_type, ETK_MEMBER_OFFSET(Etk_Widget, drag_drop),   etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_DRAG_MOTION_SIGNAL] =    etk_signal_new("drag_motion",    widget_type, ETK_MEMBER_OFFSET(Etk_Widget, drag_motion), etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_DRAG_LEAVE_SIGNAL] =     etk_signal_new("drag_leave",     widget_type, ETK_MEMBER_OFFSET(Etk_Widget, drag_leave),  etk_marshaller_VOID__VOID,    NULL, NULL);
      _etk_widget_signals[ETK_WIDGET_SELECTION_GET_SIGNAL] =  etk_signal_new("selection_get",  widget_type, -1,                                         etk_marshaller_VOID__POINTER, NULL, NULL);
#endif      
      
      etk_type_property_add(widget_type, "name",              ETK_WIDGET_NAME_PROPERTY,              ETK_PROPERTY_STRING,  ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_string(NULL));
      etk_type_property_add(widget_type, "parent",            ETK_WIDGET_PARENT_PROPERTY,            ETK_PROPERTY_POINTER, ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_pointer(NULL));
      etk_type_property_add(widget_type, "theme_file",        ETK_WIDGET_THEME_FILE_PROPERTY,        ETK_PROPERTY_STRING,  ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_string(etk_theme_widget_theme_get()));
      etk_type_property_add(widget_type, "theme_group",       ETK_WIDGET_THEME_GROUP_PROPERTY,       ETK_PROPERTY_STRING,  ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_string(NULL));
      etk_type_property_add(widget_type, "width_request",     ETK_WIDGET_WIDTH_REQUEST_PROPERTY,     ETK_PROPERTY_INT,     ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_int(-1));
      etk_type_property_add(widget_type, "height_request",    ETK_WIDGET_HEIGHT_REQUEST_PROPERTY,    ETK_PROPERTY_INT,     ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_int(-1));
      etk_type_property_add(widget_type, "visible",           ETK_WIDGET_VISIBLE_PROPERTY,           ETK_PROPERTY_BOOL,    ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_bool(ETK_FALSE));
      etk_type_property_add(widget_type, "visibility_locked", ETK_WIDGET_VISIBILITY_LOCKED_PROPERTY, ETK_PROPERTY_BOOL,    ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_bool(ETK_FALSE));
      etk_type_property_add(widget_type, "repeat_events",     ETK_WIDGET_REPEAT_EVENTS_PROPERTY,     ETK_PROPERTY_BOOL,    ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_bool(ETK_FALSE));
      etk_type_property_add(widget_type, "pass_events",       ETK_WIDGET_PASS_EVENTS_PROPERTY,       ETK_PROPERTY_BOOL,    ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_bool(ETK_FALSE));
      etk_type_property_add(widget_type, "focusable",         ETK_WIDGET_FOCUSABLE_PROPERTY,         ETK_PROPERTY_BOOL,    ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_bool(ETK_FALSE));
      etk_type_property_add(widget_type, "focus_on_press",    ETK_WIDGET_FOCUS_ON_PRESS_PROPERTY,    ETK_PROPERTY_BOOL,    ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_bool(ETK_FALSE));
      etk_type_property_add(widget_type, "can_pass_focus",    ETK_WIDGET_CAN_PASS_FOCUS_PROPERTY,    ETK_PROPERTY_BOOL,    ETK_PROPERTY_READABLE_WRITABLE, etk_property_value_bool(ETK_TRUE));

      widget_type->property_set = _etk_widget_property_set;
      widget_type->property_get = _etk_widget_property_get;
   }

   return widget_type;
}

/**
 * @brief Creates a new widget according to the type of the object
 * @param widget_type the type of the widget to create
 * @param first_property the name of the first property value
 * @param ... the value of the first argument, followed by any number of name/argument-value pairs, terminated with NULL
 * @return Returns the new Etk_Widget
 */
Etk_Widget *etk_widget_new(Etk_Type *widget_type, const char *first_property, ...)
{
   Etk_Widget *new_widget;
   va_list args;

   if (!widget_type)
      return NULL;

   va_start(args, first_property);
   new_widget = ETK_WIDGET(etk_object_new_valist(widget_type, first_property, args));
   va_end(args);

   return new_widget;
}

/**
 * @brief Sets the name of the widget
 * @param widget a widget
 * @param name the name to set
 */
void etk_widget_name_set(Etk_Widget *widget, const char *name)
{
   if (!widget || widget->name == name)
      return;

   free(widget->name);
   widget->name = strdup(name);
   etk_object_notify(ETK_OBJECT(widget), "name");
}

/**
 * @brief Gets the name of the widget
 * @param widget a widget
 * @return Returns the name of the widget
 */
const char *etk_widget_name_get(Etk_Widget *widget)
{
   if (!widget)
      return NULL;
   return widget->name;
}

/**
 * @brief Gets the geometry of the widget, relative to the top left corner of its toplevel parent
 * @param widget a widget
 * @param x the location where to set the x position of the widget
 * @param y the location where to set the y position of the widget
 * @param w the location where to set the width of the widget
 * @param h the location where to set the height of the widget
 */
void etk_widget_geometry_get(Etk_Widget *widget, int *x, int *y, int *w, int *h)
{
   if (!widget)
   {
      if (x)  *x = 0;
      if (y)  *y = 0;
      if (w)  *w = 0;
      if (h)  *h = 0;
      return;
   }
   
   if (x)  *x = widget->geometry.x;
   if (y)  *y = widget->geometry.y;
   if (w)  *w = widget->geometry.w;
   if (h)  *h = widget->geometry.h;
}

/**
 * @brief Gets the inner geometry of the widget, relative to the top left corner of its toplevel parent
 * @param widget a widget
 * @param x the location where to set the inner x position of the widget
 * @param y the location where to set the inner y position of the widget
 * @param w the location where to set the inner width of the widget
 * @param h the location where to set the inner height of the widget
 */
void etk_widget_inner_geometry_get(Etk_Widget *widget, int *x, int *y, int *w, int *h)
{
   if (!widget)
   {
      if (x)  *x = 0;
      if (y)  *y = 0;
      if (w)  *w = 0;
      if (h)  *h = 0;
      return;
   }
   
   if (x)  *x = widget->inner_geometry.x;
   if (y)  *y = widget->inner_geometry.y;
   if (w)  *w = widget->inner_geometry.w;
   if (h)  *h = widget->inner_geometry.h;
}

/**
 * @brief Gets the toplevel widget that contains @a widget
 * @param widget a widget
 * @return Returns the toplevel widget if @a widget is contained in a toplevel widget, NULL on failure
 */
Etk_Toplevel_Widget *etk_widget_toplevel_parent_get(Etk_Widget *widget)
{
   if (!widget)
      return NULL;
   return widget->toplevel_parent;
}

/**
 * @brief Gets the evas of the toplevel widget that contains @a widget
 * @param widget a widget
 * @return Returns the evas if @a widget is contained in a toplevel widget, NULL on failure
 */
Evas *etk_widget_toplevel_evas_get(Etk_Widget *widget)
{
   if (!widget || !widget->toplevel_parent)
      return NULL;
   return etk_toplevel_widget_evas_get(widget->toplevel_parent);
}

/**
 * @brief Sets the theme of @a widget
 * @param widget a widget
 * @param theme_file the path of the .edj theme file
 * @param theme_group the name of the edje group
 */ 
void etk_widget_theme_set(Etk_Widget *widget, const char *theme_file, const char *theme_group)
{
   if (!widget)
      return;

   if (theme_file != widget->theme_file)
   {
      free(widget->theme_file);
      if (theme_file)
         widget->theme_file = strdup(theme_file);
      else
         widget->theme_file = NULL;
      etk_object_notify(ETK_OBJECT(widget), "theme_file");
   }
   if (theme_group != widget->theme_group)
   {
      free(widget->theme_group);
      if (theme_group)
         widget->theme_group = strdup(theme_group);
      else
         widget->theme_group = NULL;
      etk_object_notify(ETK_OBJECT(widget), "theme_group");
   }
   
   if (widget->realized && (!widget->theme_file || !widget->theme_group))
      etk_widget_unrealize(widget);
   else if (widget->theme_file && widget->theme_group)
      etk_widget_realize(widget);
}

/**
 * @brief Realizes the widget: it will load the theme and allocate the graphical ressources
 * @param widget the widget to realize
 * @note It shouldn't be called manually, it's mainly called by widget implementations
 */
/* TODO realize all on realize */
void etk_widget_realize(Etk_Widget *widget)
{
   Evas *evas = NULL;

   if (!widget || !(evas = etk_widget_toplevel_evas_get(widget)))
      return;

   if (widget->realized)
      etk_widget_unrealize(widget);

   if (widget->theme_file && widget->theme_group)
   {
      const char *data_string = NULL;
      
      widget->theme_object = edje_object_add(evas);
      if (!edje_object_file_set(widget->theme_object, widget->theme_file, widget->theme_group))
      {
         ETK_WARNING("Can't load theme %s:%s", widget->theme_file, widget->theme_group);
         evas_object_del(widget->theme_object);
         widget->theme_object = NULL;
      }
      else
      {
         data_string = edje_object_data_get(widget->theme_object, "inset");
         if (!data_string || sscanf(data_string, "%d %d %d %d", &widget->left_inset, &widget->right_inset, &widget->top_inset, &widget->bottom_inset) != 4)
         {
            widget->left_inset = 0;
            widget->right_inset = 0;
            widget->top_inset = 0;
            widget->bottom_inset = 0;
         }
      }
   }

   widget->smart_object = _etk_widget_smart_object_add(evas, widget);
   evas_object_propagate_events_set(widget->smart_object, 0);
   evas_object_event_callback_add(widget->smart_object, EVAS_CALLBACK_MOUSE_IN, _etk_widget_mouse_in_cb, widget);
   evas_object_event_callback_add(widget->smart_object, EVAS_CALLBACK_MOUSE_OUT, _etk_widget_mouse_out_cb, widget);
   evas_object_event_callback_add(widget->smart_object, EVAS_CALLBACK_MOUSE_MOVE, _etk_widget_mouse_move_cb, widget);
   evas_object_event_callback_add(widget->smart_object, EVAS_CALLBACK_MOUSE_DOWN, _etk_widget_mouse_down_cb, widget);
   evas_object_event_callback_add(widget->smart_object, EVAS_CALLBACK_MOUSE_UP, _etk_widget_mouse_up_cb, widget);
   evas_object_event_callback_add(widget->smart_object, EVAS_CALLBACK_MOUSE_WHEEL, _etk_widget_mouse_wheel_cb, widget);
   evas_object_event_callback_add(widget->smart_object, EVAS_CALLBACK_KEY_DOWN, _etk_widget_key_down_cb, widget);
   evas_object_event_callback_add(widget->smart_object, EVAS_CALLBACK_KEY_UP, _etk_widget_key_up_cb, widget);
   if (widget->visible)
      evas_object_show(widget->smart_object);
   else
      evas_object_hide(widget->smart_object);

   if (widget->theme_object)
   {
      evas_object_show(widget->theme_object);
      etk_widget_member_object_add(widget, widget->theme_object);
   }
   
   if (widget->clip)
   {
      evas_object_show(widget->clip);
      evas_object_clip_set(widget->smart_object, widget->clip);
   }

   if (ETK_IS_CONTAINER(widget))
   {
      Evas_List *l;
      Etk_Widget *child;

      for (l = ETK_CONTAINER(widget)->children; l; l = l->next)
      {
         child = l->data;
         if (!child->swallowed && child->smart_object)
            etk_widget_member_object_add(widget, child->smart_object);
      }
   }

   if (widget->parent && ETK_WIDGET(widget->parent)->smart_object && !widget->swallowed)
      etk_widget_member_object_add(ETK_WIDGET(widget->parent), widget->smart_object);

   evas_object_repeat_events_set(widget->smart_object, widget->repeat_events);
   evas_object_pass_events_set(widget->smart_object, widget->pass_events);

   widget->need_theme_min_size_recalc = ETK_TRUE;
   widget->realized = ETK_TRUE;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_REALIZE_SIGNAL], ETK_OBJECT(widget), NULL);
   etk_widget_size_recalc_queue(widget);
}

/**
 * @brief Unrealizes the widget: it will unload the theme and free the graphical ressources
 * @param widget the widget to unrealize
 * @note It shouldn't be called manually, it's mainly called by widget implementations
 */
void etk_widget_unrealize(Etk_Widget *widget)
{
   Etk_Widget_Member_Object *m;
   Etk_Widget_Swallowed_Object *swallowed_object;

   if (!widget || !widget->realized)
      return;
   
   if (widget->parent && widget->smart_object)
      etk_widget_member_object_del(ETK_WIDGET(widget->parent), widget->smart_object);

   while (widget->swallowed_objects)
   {
      swallowed_object = widget->swallowed_objects->data;
      etk_widget_theme_object_unswallow(widget, swallowed_object->object);
   }

   widget->theme_object = NULL;
   while (widget->member_objects)
   {
      m = widget->member_objects->data;
      if (!evas_object_data_get(m->object, "etk_widget"))
         evas_object_del(m->object);
      free(m);
      widget->member_objects = evas_list_remove_list(widget->member_objects, widget->member_objects);
   }
   evas_object_del(widget->smart_object);
   widget->smart_object = NULL;

   if (widget->clip)
      evas_object_hide(widget->clip);

   widget->left_inset = 0;
   widget->right_inset = 0;
   widget->top_inset = 0;
   widget->bottom_inset = 0;
   
   widget->realized = ETK_FALSE;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_UNREALIZE_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Set the parent of the of the widget but do not add it to the container parent @n
 * Used mainly by widget implementations
 * @param widget a widget
 * @param parent the new parent
 */
void etk_widget_parent_set(Etk_Widget *widget, Etk_Container *parent)
{
   Etk_Container *old_parent;
   Etk_Toplevel_Widget *old_toplevel;
   Evas *old_evas, *new_evas;
   Etk_Widget *toplevel;

   if (!widget)
      return;

   if (parent)
   {
      parent->children = evas_list_append(parent->children, widget);
      etk_signal_emit_by_name("child_added", ETK_OBJECT(parent), NULL, widget);
   }
   else if (widget->parent)
   {
      widget->parent->children = evas_list_remove(widget->parent->children, widget);
      etk_signal_emit_by_name("child_removed", ETK_OBJECT(widget->parent), NULL, widget);
   }

   old_parent = widget->parent;
   widget->parent = parent;

   old_toplevel = widget->toplevel_parent;
   for (toplevel = widget; toplevel->parent; toplevel = ETK_WIDGET(toplevel->parent));
   if (ETK_IS_TOPLEVEL_WIDGET(toplevel))
      widget->toplevel_parent = ETK_TOPLEVEL_WIDGET(toplevel);
   else
      widget->toplevel_parent = NULL;

   if ((widget->toplevel_parent != old_toplevel))
      _etk_widget_toplevel_parent_set(widget, widget->toplevel_parent);

   old_evas = etk_toplevel_widget_evas_get(old_toplevel);
   new_evas = etk_toplevel_widget_evas_get(widget->toplevel_parent);

   if (new_evas && (!widget->realized || (old_evas != new_evas)))
      _etk_widget_realize_all(widget);
   else if (!new_evas && widget->realized)
      _etk_widget_unrealize_all(widget);
   else
      etk_widget_size_recalc_queue(widget);

   if (!widget->swallowed && widget->smart_object)
   {
      if (parent)
         etk_widget_member_object_add(ETK_WIDGET(parent), widget->smart_object);
      else if (old_parent)
         etk_widget_member_object_del(ETK_WIDGET(parent), widget->smart_object);
   }

   etk_object_notify(ETK_OBJECT(widget), "parent");
}

/**
 * @brief Reparents the widget
 * @param widget a widget
 * @param parent the new parent
 */
void etk_widget_reparent(Etk_Widget *widget, Etk_Container *parent)
{
   if (!widget)
      return;

   if (parent)
      etk_container_add(parent, widget);
   else if (widget->parent)
      etk_container_remove(widget->parent, widget);
}

/**
 * @brief Gets the child properties of the widget (the type of the returned value depends on the parent container type)
 * @param widget a widget
 * @return Returns the child properties of the widget, NULL on failure
 * @note You can modify the value of a child property, but you'll certainly have to queue a resize or a redraw on the widget to see the change applied
 */ 
void *etk_widget_child_properties_get(Etk_Widget *widget)
{
   if (!widget)
      return NULL;
   return widget->child_properties;
}

/**
 * @brief Sets if the widget should repeat the events it receives
 * @param widget a widget
 * @param repeat_events if @a repeat_events == ETK_TRUE, the widget below @a widget will receive also the mouse events
 */
void etk_widget_repeat_events_set(Etk_Widget *widget, Etk_Bool repeat_events)
{
   if (!widget)
      return;

   widget->repeat_events = repeat_events;
   if (widget->smart_object)
      evas_object_repeat_events_set(widget->smart_object, repeat_events);
   etk_object_notify(ETK_OBJECT(widget), "repeat_events");
}

/**
 * @brief Checks if the widget repeats the events it receives
 * @param widget a widget
 * @return Returns ETK_TRUE if the widget repeats the events it receives
 */
Etk_Bool etk_widget_repeat_events_get(Etk_Widget *widget)
{
   if (!widget)
      return ETK_FALSE;
   return widget->repeat_events;
}

/**
 * @brief Sets if the widget should pass the events it receives
 * @param widget a widget
 * @param pass_events if @a pass_events == ETK_TRUE, the widget below @a widget will receive the mouse events and @a widget will no longer receive mouse events
 */
void etk_widget_pass_events_set(Etk_Widget *widget, Etk_Bool pass_events)
{
   if (!widget)
      return;

   widget->pass_events = pass_events;
   if (widget->smart_object)
      evas_object_pass_events_set(widget->smart_object, pass_events);
   etk_object_notify(ETK_OBJECT(widget), "pass_events");
}

/**
 * @brief Checks if the widget passes the events it receives
 * @param widget a widget
 * @return Returns ETK_TRUE if the widget passes the events it receives
 */
Etk_Bool etk_widget_pass_events_get(Etk_Widget *widget)
{
   if (!widget)
      return ETK_FALSE;
   return widget->pass_events;
}

/**
 * @brief Stops the propagation of the current event. The parents of the widget that has received the event won't be notified
 */
void etk_widget_event_propagation_stop()
{
   _etk_widget_propagate_event = ETK_FALSE;
}

/**
 * @brief Shows the widget. The widget will effectively be shown if all its parents are also shown
 * @param widget a widget
 */
void etk_widget_show(Etk_Widget *widget)
{
   if (!widget || widget->visible)
      return;

   widget->visible = ETK_TRUE;
   if (widget->smart_object)
      evas_object_show(widget->smart_object);
   etk_widget_size_recalc_queue(widget);
   etk_object_notify(ETK_OBJECT(widget), "visible");
   
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_SHOW_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Recursively shows the widget and its child (if it's a container)
 * @param widget a widget
 */
void etk_widget_show_all(Etk_Widget *widget)
{
   if (!widget)
      return;

   if (!widget->visibility_locked)
      etk_widget_show(widget);
   if (ETK_IS_CONTAINER(widget))
      etk_container_for_each(ETK_CONTAINER(widget), etk_widget_show_all);
}

/**
 * @brief Hides the widget
 * @param widget a widget
 */
void etk_widget_hide(Etk_Widget *widget)
{
   if (!widget || !widget->visible)
      return;

   widget->visible = ETK_FALSE;
   if (widget->smart_object)
      evas_object_hide(widget->smart_object);
   etk_widget_size_recalc_queue(widget);
   etk_object_notify(ETK_OBJECT(widget), "visible");
   
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_HIDE_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Recursively hides the widget and its child (if it's a container)
 * @param widget a widget
 */
void etk_widget_hide_all(Etk_Widget *widget)
{
   if (!widget)
      return;

   if (!widget->visibility_locked)
      etk_widget_hide(widget);
   if (ETK_IS_CONTAINER(widget))
      etk_container_for_each(ETK_CONTAINER(widget), etk_widget_hide_all);
}

/**
 * @brief Checks if the object is visible
 * @param widget a widget
 * @return Returns ETK_TRUE if the widget is visible, ETK_FALSE otherwise
 */
Etk_Bool etk_widget_is_visible(Etk_Widget *widget)
{
   if (!widget)
      return ETK_FALSE;
   return widget->visible;
}

/**
 * @brief Sets whether the widget is affected by etk_widget_show_all() and etk_widget_hide_all(). It's mainly used for implementation of widgets
 * @param widget a widget
 * @param visibility_locked ETK_TRUE if etk_widget_show_all() and etk_widget_hide_all() should not affect the visibility of the widget
 */
void etk_widget_visibility_locked_set(Etk_Widget *widget, Etk_Bool visibility_locked)
{
   if (!widget || widget->visibility_locked == visibility_locked)
      return;
   widget->visibility_locked = visibility_locked;
   etk_object_notify(ETK_OBJECT(widget), "visibility_locked");
}

/**
 * @brief Gets whether the widget is affected by etk_widget_show_all() and etk_widget_hide_all()
 * @param widget a widget
 * @return Returns ETK_TRUE if etk_widget_show_all() and etk_widget_hide_all() does not affect the visibility of the widget
 */
Etk_Bool etk_widget_visibility_locked_get(Etk_Widget *widget)
{
   if (!widget)
      return ETK_FALSE;
   return widget->visibility_locked;
}

/**
 * @brief Queues a size recalculation request: during the next mainloop iteration, the widget size will be recalculated@n
 * It's mainly used in widget implementations
 * @param widget the widget to queue
 */
void etk_widget_size_recalc_queue(Etk_Widget *widget)
{
   Etk_Widget *w;

   if (!widget)
      return;

   for (w = widget; w; w = ETK_WIDGET(w->parent))
   {
      if (w->swallowed && w->smart_object)
      {
         Etk_Widget_Smart_Data *smart_data;

         if ((smart_data = evas_object_smart_data_get(w->smart_object)) && smart_data->swallowing_widget)
            smart_data->swallowing_widget->need_theme_min_size_recalc = ETK_TRUE;
      }
      w->need_size_recalc = ETK_TRUE;
      w->need_redraw = ETK_TRUE;
   }

   if (widget->toplevel_parent)
      etk_main_iteration_queue();
}

/**
 * @brief Queues a redraw request: during the next mainloop iteration, the widget will be redrawn@n
 * It's mainly used in widget implementations
 * @param widget the widget to queue
 */
void etk_widget_redraw_queue(Etk_Widget *widget)
{
   if (!widget)
      return;

   _etk_widget_redraw_queue_recursive(widget);
   if (widget->toplevel_parent)
      etk_main_iteration_queue();
}

/**
 * @brief Set the ideal size of the widget. The size set will be used for the next size requests instead of calculating it
 * @param widget a widget
 * @param w the ideal width (-1 will make the widget calculate it)
 * @param h the ideal height (-1 will make the widget calculate it)
 */
void etk_widget_size_request_set(Etk_Widget *widget, int w, int h)
{
   if (!widget)
      return;

   widget->requested_size.w = w;
   widget->requested_size.h = h;

   etk_object_notify(ETK_OBJECT(widget), "width_request");
   etk_object_notify(ETK_OBJECT(widget), "height_request");
   etk_widget_size_recalc_queue(widget);
}

/**
 * @brief Calculates the ideal size of the widget. Used mainly for container implementation
 * @param widget a widget
 * @param size_requisition the location where to set the result
 */
void etk_widget_size_request(Etk_Widget *widget, Etk_Size *size_requisition)
{
   etk_widget_size_request_full(widget, size_requisition, ETK_TRUE);
}

/**
 * @brief Calculates the ideal size of the widget. Used mainly for container implementation
 * @param widget a widget
 * @param size_requisition the location where to set the result
 * @param hidden_has_no_size if hidden_has_no_size is ETK_TRUE, then if the widget is hidden, the size requisition will be 0x0
 */
void etk_widget_size_request_full(Etk_Widget *widget, Etk_Size *size_requisition, Etk_Bool hidden_has_no_size)
{
   if (!widget || !size_requisition)
      return;

   size_requisition->w = -1;
   size_requisition->h = -1;

   if (!widget->visible && hidden_has_no_size)
      size_requisition->w = 0;
   else if (!widget->need_size_recalc && widget->last_size_requisition.w >= 0)
      size_requisition->w = widget->last_size_requisition.w;
   else if (widget->requested_size.w >= 0)
      size_requisition->w = widget->requested_size.w;
   
   if (!widget->visible && hidden_has_no_size)
      size_requisition->h = 0;
   else if (!widget->need_size_recalc && widget->last_size_requisition.h >= 0)
      size_requisition->h = widget->last_size_requisition.h;
   else if (widget->requested_size.h >= 0)
      size_requisition->h = widget->requested_size.h;

   if (size_requisition->w < 0 || size_requisition->h < 0)
   {
      int min_w, min_h;

      etk_widget_theme_object_min_size_calc(widget, &min_w, &min_h);
      if (widget->size_request)
      {
         Etk_Size widget_requisition;

         widget->size_request(widget, &widget_requisition);
         if (size_requisition->w < 0)
            size_requisition->w = ETK_MAX(min_w, widget_requisition.w + widget->left_inset + widget->right_inset);
         if (size_requisition->h < 0)
            size_requisition->h = ETK_MAX(min_h, widget_requisition.h + widget->top_inset + widget->bottom_inset);
      }
      else
      {
         if (size_requisition->w < 0)
            size_requisition->w = min_w;
         if (size_requisition->h < 0)
            size_requisition->h = min_h;
      }
   }

   if (widget->visible)
      widget->last_size_requisition = *size_requisition;
   widget->need_size_recalc = ETK_FALSE;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_SIZE_REQUEST_SIGNAL], ETK_OBJECT(widget), NULL, size_requisition);
}

/**
 * @brief Moves and resizes the theme object and sets the size of the widget according to @a geometry. Used mainly for container implementation
 * @param widget a widget
 * @param geometry the size the widget should have
 */
void etk_widget_size_allocate(Etk_Widget *widget, Etk_Geometry geometry)
{
   if (!widget || widget->swallowed || !widget->smart_object)
      return;

   if (geometry.x != widget->geometry.x || geometry.y != widget->geometry.y)
      evas_object_move(widget->smart_object, geometry.x, geometry.y);
   if (geometry.w != widget->geometry.w || geometry.h != widget->geometry.h || widget->need_redraw)
      evas_object_resize(widget->smart_object, geometry.w, geometry.h);
   
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_SIZE_ALLOCATE_SIGNAL], ETK_OBJECT(widget), NULL, &geometry);
}

/**
 * @brief Emits the "enter" signal on the widget
 * @param widget a widget
 */
void etk_widget_enter(Etk_Widget *widget)
{
   if (!widget)
      return;
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_ENTER_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Emits the "leave" signal on the widget
 * @param widget a widget
 */
void etk_widget_leave(Etk_Widget *widget)
{
   if (!widget)
      return;
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_LEAVE_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Focuses the widget
 * @param widget a widget
 */
void etk_widget_focus(Etk_Widget *widget)
{
   Etk_Widget *focused;

   if (!widget || !widget->toplevel_parent || !widget->focusable)
      return;
   if ((focused = etk_toplevel_widget_focused_widget_get(widget->toplevel_parent)) && (widget == focused))
      return;

   if (focused)
      etk_widget_unfocus(focused);

   etk_toplevel_widget_focused_widget_set(widget->toplevel_parent, widget);
   if (widget->smart_object)
      evas_object_focus_set(widget->smart_object, 1);
   
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_FOCUS_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Unfocuses the widget
 * @param widget a widget
 */
void etk_widget_unfocus(Etk_Widget *widget)
{
   Etk_Widget *focused;

   if (!widget || !widget->toplevel_parent || !(focused = etk_toplevel_widget_focused_widget_get(widget->toplevel_parent)) || (focused != widget))
      return;

   etk_toplevel_widget_focused_widget_set(widget->toplevel_parent, NULL);
   if (widget->smart_object)
      evas_object_focus_set(widget->smart_object, 0);
   
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_UNFOCUS_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Makes the widget swallow another widget in a part of its theme object. Used mainly for widget implementation
 * @param swallowing_widget the widget that will swallow @a widget_to_swallow
 * @param part the name of the part of the theme object that will swallow @a widget_to_swallow
 * @param widget_to_swallow the widget to swallow
 * @return Returns ETK_TRUE on success, ETK_FALSE on failure (generally because the part doesn't exists, or because @a swallowing_widget isn't realized)
 * @note The swallowed widget has to be a child of the swallowing widget. Otherwise, it won't be swallowed and it will return ETK_FALSE
 */
Etk_Bool etk_widget_swallow_widget(Etk_Widget *swallowing_widget, const char *part, Etk_Widget *widget_to_swallow)
{
   Evas *evas;
   Etk_Widget_Smart_Data *smart_data;

   if (!swallowing_widget || !part || !widget_to_swallow || !swallowing_widget->theme_object || !widget_to_swallow->smart_object)
      return ETK_FALSE;
   if (!(evas = etk_widget_toplevel_evas_get(swallowing_widget)) || (evas != etk_widget_toplevel_evas_get(widget_to_swallow)))
      return ETK_FALSE;
   if (!edje_object_part_exists(swallowing_widget->theme_object, part))
      return ETK_FALSE;
   if (!ETK_IS_CONTAINER(swallowing_widget) || !etk_container_is_child(ETK_CONTAINER(swallowing_widget), widget_to_swallow))
   {
      ETK_WARNING("Unable to \"swallow\" the widget %p by the widget %p because the widget "
         "to swallow was not a child of the \"swallowing\" widget\n", widget_to_swallow, swallowing_widget);
      return ETK_FALSE;
   }

   if ((smart_data = evas_object_smart_data_get(widget_to_swallow->smart_object)) && smart_data->swallowing_widget)
      etk_widget_unswallow_widget(smart_data->swallowing_widget, widget_to_swallow);

   widget_to_swallow->swallowed = ETK_TRUE;
   if (!_etk_widget_theme_object_swallow_full(swallowing_widget, part, widget_to_swallow->smart_object, ETK_TRUE))
      widget_to_swallow->swallowed = ETK_FALSE;

   return ETK_FALSE;
}

/**
 * @brief Makes the widget unswallow another widget
 * @param swallowing_widget the widget that will unswallow @a widget
 * @param widget the widget to unswallow
 */
void etk_widget_unswallow_widget(Etk_Widget *swallowing_widget, Etk_Widget *widget)
{
   if (!swallowing_widget || !widget || !widget->smart_object)
      return;

   etk_widget_theme_object_unswallow(swallowing_widget, widget->smart_object);
}

/**
 * @brief Checks if the widget swallows the other widget
 * @param widget a widget
 * @param swallowed_widget the widget which we are checking if it is swallowed by @a widget
 * @return Returns ETK_TRUE if @a widget is swallowing @a swallowed_widget
 */
Etk_Bool etk_widget_widget_is_swallowed(Etk_Widget *widget, Etk_Widget *swallowed_widget)
{
   if (!widget || !swallowed_widget || !swallowed_widget->smart_object)
      return ETK_FALSE;
   return etk_widget_object_is_swallowed(widget, swallowed_widget->smart_object);
}

/**
 * @brief Makes the theme object of @a swallowing_widget swallow @a object in a part called @a part
 * @param swallowing_widget the widget that will swallow the object
 * @param part the name of the part
 * @param object the object to swallow
 * @return Returns ETK_TRUE on success, ETK_FALSE on failure (generally because the part doesn't exists, or because @a swallowing_widget isn't realized)
 */
Etk_Bool etk_widget_theme_object_swallow(Etk_Widget *swallowing_widget, const char *part, Evas_Object *object)
{
   return _etk_widget_theme_object_swallow_full(swallowing_widget, part, object, ETK_FALSE);
}

/**
 * @brief Makes the theme object of @a swallowing_widget unswallow @a object
 * @param swallowing_widget the widget that will unswallow the object
 * @param object the object to unswallow
 */
void etk_widget_theme_object_unswallow(Etk_Widget *swallowing_widget, Evas_Object *object)
{
   Evas_List *l;
   Etk_Widget_Swallowed_Object *swallowed_object = NULL;
   Etk_Widget_Smart_Data *smart_data;

   if (!swallowing_widget || !object || !swallowing_widget->theme_object)
      return;

   edje_object_part_unswallow(swallowing_widget->theme_object, object);

   for (l = swallowing_widget->swallowed_objects; l; l = l->next)
   {
      swallowed_object = l->data;
      if (swallowed_object->object == object)
         break;
   }
   if (!swallowed_object || swallowed_object->object != object)
      return;

   if (swallowed_object->is_widget_theme_object && (smart_data = evas_object_smart_data_get(swallowed_object->object)) && smart_data->widget)
      smart_data->widget->swallowed = ETK_FALSE;

   _etk_widget_swallowed_object_free(swallowed_object);
   swallowing_widget->swallowed_objects = evas_list_remove_list(swallowing_widget->swallowed_objects, l);

   etk_widget_member_object_add(swallowing_widget, object);

   etk_widget_size_recalc_queue(swallowing_widget);
}

/**
 * @brief Checks if the widget swallows the object
 * @param widget a widget
 * @param object an evas object
 * @return Returns ETK_TRUE if @a widget is swallowing @a object
 */
Etk_Bool etk_widget_object_is_swallowed(Etk_Widget *widget, Evas_Object *object)
{
   Evas_List *l;
   Etk_Widget_Swallowed_Object *swallowed_object;

   if (!widget || !object)
      return ETK_FALSE;

   for (l = widget->swallowed_objects; l; l = l->next)
   {
      swallowed_object = l->data;
      if (swallowed_object->object == object)
         return ETK_TRUE;
   }
   return ETK_FALSE;
}

/**
 * @brief Calculates the minimum size of the theme object of the widget. Used mainly for widget implementations
 * @param widget a widget
 * @param w the location where to set the calculated width
 * @param h the location where to set the calculated height
 */
void etk_widget_theme_object_min_size_calc(Etk_Widget *widget, int *w, int *h)
{
   if (!widget)
      return;

   if (widget->need_theme_min_size_recalc)
   {
      if (widget->theme_object)
      {
         int min_calc_width, min_calc_height;
         int min_get_width, min_get_height;
         Evas_List *l;
         Etk_Widget_Swallowed_Object *swallowed_object;

         for (l = widget->swallowed_objects; l; l = l->next)
         {
            swallowed_object = l->data;
            if (swallowed_object->is_widget_theme_object)
            {
               Etk_Widget_Smart_Data *smart_data;
               Etk_Size swallow_size;

               if ((smart_data = evas_object_smart_data_get(swallowed_object->object)))
               {
                  etk_widget_size_request(smart_data->widget, &swallow_size);
                  edje_extern_object_min_size_set(smart_data->widget->smart_object, swallow_size.w, swallow_size.h);
                  edje_object_part_swallow(widget->theme_object, swallowed_object->swallowing_part, swallowed_object->object);
               }
            }
         }
         edje_object_message_signal_process(widget->theme_object);
         edje_object_size_min_calc(widget->theme_object, &min_calc_width, &min_calc_height);
         edje_object_size_min_get(widget->theme_object, &min_get_width, &min_get_height);
         widget->theme_min_width = ETK_MAX(min_calc_width, min_get_width);
         widget->theme_min_height = ETK_MAX(min_calc_height, min_get_height);

         for (l = widget->swallowed_objects; l; l = l->next)
         {
            swallowed_object = l->data;
            if (swallowed_object->is_widget_theme_object)
            {
               Etk_Widget_Smart_Data *smart_data;

               if ((smart_data = evas_object_smart_data_get(swallowed_object->object)))
               {
                  edje_extern_object_min_size_set(smart_data->widget->smart_object, 0, 0);
                  edje_object_part_swallow(widget->theme_object, swallowed_object->swallowing_part, swallowed_object->object);
               }
            }
         }
      }
      else
      {
         widget->theme_min_width = 0;
         widget->theme_min_height = 0;
      }
      
      widget->theme_min_width = ETK_MAX(widget->theme_min_width, widget->left_inset + widget->right_inset);
      widget->theme_min_height = ETK_MAX(widget->theme_min_height, widget->top_inset + widget->bottom_inset);
      widget->need_theme_min_size_recalc = ETK_FALSE;
   }

   if (w)
      *w = widget->theme_min_width;
   if (h)
      *h = widget->theme_min_height;
}

/**
 * @brief Sends a signal to the theme object of the widget
 * @param widget a widget
 * @param signal_name the name of the signal to send
 */
void etk_widget_theme_object_signal_emit(Etk_Widget *widget, const char *signal_name)
{
   if (!widget || !widget->theme_object)
      return;

   edje_object_signal_emit(widget->theme_object, signal_name, "");
   widget->need_theme_min_size_recalc = ETK_TRUE;
}

/**
 * @brief Sets a the text of a text part of the theme object of the widget
 * @param widget a widget
 * @param part_name the name of the text part
 * @param text the text to set
 */
void etk_widget_theme_object_part_text_set(Etk_Widget *widget, const char *part_name, const char *text)
{
   if (!widget || !widget->theme_object)
      return;

   edje_object_part_text_set(widget->theme_object, part_name, text);
   widget->need_theme_min_size_recalc = ETK_TRUE;
}

/**
 * @brief Gets the value of data from the theme of the widget. The widget has to be realized
 * @param widget a widget
 * @param data_name the name of the data you want to get the value
 * @param format the format of the data. Same format than the format argument of sscanf
 * @param ... the location of the variables where to set the values
 * @return Returns the number of the input items successfully matched and assigned, same as sscanf
 */
int etk_widget_theme_object_data_get(Etk_Widget *widget, const char *data_name, const char *format, ...)
{
   const char *data_string;
   int result;
   va_list args;
   
   if (!widget || !data_name || !format || !widget->theme_object)
      return 0;
   if (!(data_string = edje_object_data_get(widget->theme_object, data_name)))
      return 0;
   
   va_start(args, format);
   result = vsscanf(data_string, format, args);
   va_end(args);
   
   return result;
}

/**
 * @brief Adds an evas object to the list of objet members of the widget. Used for widget implementations mainly @n
 * The object will be automatically deleted when the object will be unrealized. Used for widget implementations mainly
 * @param widget a widget
 * @param object the evas object to add
 * @return Returns ETK_TRUE on success. ETK_FALSE on failure, probably because the widget and the object do not belong to the same evas
 */
Etk_Bool etk_widget_member_object_add(Etk_Widget *widget, Evas_Object *object)
{
   Etk_Widget_Member_Object *member_object;
   
   if (!widget || !object || !widget->smart_object || (evas_object_evas_get(object) != etk_widget_toplevel_evas_get(widget)))
      return ETK_FALSE;
   if (etk_widget_object_is_swallowed(widget, object))
      return ETK_FALSE;
   if (_etk_widget_member_object_find(widget, object))
      return ETK_TRUE;
   
   member_object = malloc(sizeof(Etk_Widget_Member_Object));
   member_object->object = object;
   member_object->visible = evas_object_visible_get(object);
   
   evas_object_smart_member_add(object, widget->smart_object);
   evas_object_raise(object);
   if (!evas_object_visible_get(widget->smart_object))
      evas_object_hide(object);
   evas_object_intercept_show_callback_add(object, _etk_widget_member_object_intercept_show_cb, widget);
   evas_object_intercept_hide_callback_add(object, _etk_widget_member_object_intercept_hide_cb, widget);
   
   if (widget->clip)
      evas_object_clip_set(object, widget->clip);
   
   widget->member_objects = evas_list_append(widget->member_objects, member_object);

   return ETK_TRUE;
}

/**
 * @brief Removes an evas object from the list of objet members of the widget. Used for widget implementations mainly
 * @param widget a widget
 * @param object the evas object to remove
 */
void etk_widget_member_object_del(Etk_Widget *widget, Evas_Object *object)
{
   Evas_List *l;
   Etk_Widget_Member_Object *member_object;

   if (!widget || !object)
      return;

   if ((l = _etk_widget_member_object_find(widget, object)))
   {
      member_object = l->data;
      widget->member_objects = evas_list_remove_list(widget->member_objects, l);
      
      evas_object_smart_member_del(member_object->object);
      if (widget->clip)
         evas_object_clip_unset(member_object->object);
      evas_object_intercept_show_callback_del(member_object->object, _etk_widget_member_object_intercept_show_cb);
      evas_object_intercept_hide_callback_del(member_object->object, _etk_widget_member_object_intercept_hide_cb);
         
      free(member_object);
   }
}

/**
 * @brief Stacks @a object above all the other member objects of the widget
 * @param widget a widget
 * @param object the object to raise
 */
void etk_widget_member_object_raise(Etk_Widget *widget, Evas_Object *object)
{
   if (!widget || !object)
      return;

   if (_etk_widget_member_object_find(widget, object))
      evas_object_raise(object);
}

/**
 * @brief Stacks @a object below all the other member objects of the widget
 * @param widget a widget
 * @param object the object to lower
 */
void etk_widget_member_object_lower(Etk_Widget *widget, Evas_Object *object)
{
   if (!widget || !object)
      return;

   if (_etk_widget_member_object_find(widget, object))
      evas_object_lower(object);
}

/**
 * @brief Stacks the member object @a object above the member object @a above
 * @param widget a widget
 * @param object the object to restack
 * @param above the object above which @a object will be stacked 
 */
void etk_widget_member_object_stack_above(Etk_Widget *widget, Evas_Object *object, Evas_Object *above)
{
   if (!widget || !above || (object == above))
      return;

   if (_etk_widget_member_object_find(widget, object) && _etk_widget_member_object_find(widget, above))
      evas_object_stack_above(object, above);
}

/**
 * @brief Stacks the member object @a object below the member object @a below
 * @param widget a widget
 * @param object the object to restack
 * @param below the object below which @a object will be stacked 
 */
void etk_widget_member_object_stack_below(Etk_Widget *widget, Evas_Object *object, Evas_Object *below)
{
   if (!widget || !below || (object == below))
      return;

   if (_etk_widget_member_object_find(widget, object) && _etk_widget_member_object_find(widget, below))
      evas_object_stack_below(object, below);
}

/**
 * @brief Sets the clip object of the widget. Used mainly for widget implementations
 * @param widget a widget
 * @param clip the clip object to set
 * @warning when the clip is destoyed, you have to call etk_widget_clip_unset() on the widget
 */
void etk_widget_clip_set(Etk_Widget *widget, Evas_Object *clip)
{
   if (!widget)
      return;

   if (widget->clip)
      etk_widget_clip_unset(widget);
   
   if (widget->smart_object)
   {
      evas_object_show(clip);
      evas_object_clip_set(widget->smart_object, clip);
   }
   else
   {
      widget->clip = clip;
      evas_object_hide(widget->clip);
   }
}

/**
 * @brief Unsets the clip object of the widget. Used mainly for widget implementations
 * @param widget a widget
 */
void etk_widget_clip_unset(Etk_Widget *widget)
{
   if (!widget || !widget->clip)
      return;

   if (widget->smart_object)
      evas_object_clip_unset(widget->smart_object);
   widget->clip = NULL;
}

/**
 * @brief Gets the clip object of the widget
 * @param widget a widget
 * @return Returns the clip object of the widget
 */
Evas_Object *etk_widget_clip_get(Etk_Widget *widget)
{
   if (!widget)
      return NULL;
   return widget->clip;
}

#if HAVE_ECORE_X
/**
 * @brief Sends the "drag_drop" signal
 * @param widget a widget
 */
void etk_widget_drag_drop(Etk_Widget *widget)
{
   if (!widget)
     return;
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_DRAG_DROP_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Sends the "drag_motion" signal
 * @param widget a widget
 */
void etk_widget_drag_motion(Etk_Widget *widget)
{
   if (!widget)
     return;
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_DRAG_MOTION_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Sends the "drag_motion" signal
 * @param widget a widget
 */
void etk_widget_drag_leave(Etk_Widget *widget)
{
   if (!widget)
     return;
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_DRAG_LEAVE_SIGNAL], ETK_OBJECT(widget), NULL);
}

/**
 * @brief Sends the "selection_get" signal
 * @param widget a widget
 */
void etk_widget_selection_get(Etk_Widget *widget, Etk_Event_Selection_Get *event)
{
   if (!widget)
     return;

   printf("%s <=-\n", (char*)event->data);
   
   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_SELECTION_GET_SIGNAL], ETK_OBJECT(widget), NULL, event);
}

#endif

/**************************
 *
 * Etk specific functions
 *
 **************************/

/* Initializes the default values of the widget */
static void _etk_widget_constructor(Etk_Widget *widget)
{
   if (!widget)
      return;

   widget->name = NULL;
   widget->parent = NULL;
   widget->toplevel_parent = NULL;
   widget->child_properties = NULL;

   widget->theme_object = NULL;
   widget->theme_min_width = 0;
   widget->theme_min_height = 0;
   widget->theme_file = strdup(etk_theme_widget_theme_get());
   widget->theme_group = NULL;

   widget->smart_object = NULL;
   widget->clip = NULL;
   widget->swallowed_objects = NULL;

   widget->member_objects = NULL;
   widget->show = _etk_widget_show_handler;
   widget->enter = _etk_widget_enter_handler;
   widget->leave = _etk_widget_leave_handler;
   widget->focus = _etk_widget_focus_handler;
   widget->unfocus = _etk_widget_unfocus_handler;
   widget->drag_drop = _etk_widget_drag_drop_handler;
   widget->drag_motion = _etk_widget_drag_motion_handler;
   widget->drag_leave = _etk_widget_drag_leave_handler;   
   
   widget->left_inset = 0;
   widget->right_inset = 0;
   widget->top_inset = 0;
   widget->bottom_inset = 0;

   memset(&widget->geometry, -1, sizeof(Etk_Geometry));
   memset(&widget->inner_geometry, -1, sizeof(Etk_Geometry));
   widget->size_request = NULL;
   widget->size_allocate = NULL;
   widget->requested_size.w = -1;
   widget->requested_size.h = -1;
   widget->last_size_requisition.w = 0;
   widget->last_size_requisition.h = 0;

   widget->scroll_size_get = NULL;
   widget->scroll_margins_get = NULL;
   widget->scroll = NULL;

   widget->realized = ETK_FALSE;
   widget->visible = ETK_FALSE;
   widget->visibility_locked = ETK_FALSE;
   widget->focusable = ETK_FALSE;
   widget->focus_on_press = ETK_FALSE;
   widget->can_pass_focus = ETK_TRUE;
   widget->repeat_events = ETK_FALSE;
   widget->pass_events = ETK_FALSE;
   widget->need_size_recalc = ETK_FALSE;
   widget->need_redraw = ETK_FALSE;
   widget->need_theme_min_size_recalc = ETK_FALSE;
   widget->swallowed = ETK_FALSE;
#if HAVE_ECORE_X   
   widget->accepts_xdnd = ETK_FALSE;
   widget->xdnd_files = NULL;
   widget->xdnd_files_num = 0;
#endif   
   
   etk_signal_connect_full(_etk_widget_signals[ETK_WIDGET_MOUSE_IN_SIGNAL], ETK_OBJECT(widget), ETK_CALLBACK(_etk_widget_signal_mouse_in_cb), NULL, ETK_FALSE, ETK_FALSE);
   etk_signal_connect_full(_etk_widget_signals[ETK_WIDGET_MOUSE_OUT_SIGNAL], ETK_OBJECT(widget), ETK_CALLBACK(_etk_widget_signal_mouse_out_cb), NULL, ETK_FALSE, ETK_FALSE);
   etk_signal_connect_full(_etk_widget_signals[ETK_WIDGET_MOUSE_DOWN_SIGNAL], ETK_OBJECT(widget), ETK_CALLBACK(_etk_widget_signal_mouse_down_cb), NULL, ETK_FALSE, ETK_FALSE);
   etk_signal_connect_full(_etk_widget_signals[ETK_WIDGET_KEY_DOWN_SIGNAL], ETK_OBJECT(widget), ETK_CALLBACK(_etk_widget_signal_key_down_cb), NULL, ETK_FALSE, ETK_FALSE);
}

/* Destroys the widget */
static void _etk_widget_destructor(Etk_Widget *widget)
{
   if (!widget)
      return;

   etk_widget_unrealize(widget);
   etk_container_remove(widget->parent, widget);
   if (widget->parent)
      etk_widget_parent_set(widget, NULL);
   
   free(widget->theme_file);
   free(widget->theme_group);
}

/* Sets the property whose id is "property_id" to the value "value" */
static void _etk_widget_property_set(Etk_Object *object, int property_id, Etk_Property_Value *value)
{
   Etk_Widget *widget;

   if (!(widget = ETK_WIDGET(object)) || !value)
      return;

   switch (property_id)
   {
      case ETK_WIDGET_THEME_FILE_PROPERTY:
         etk_widget_theme_set(widget, etk_property_value_string_get(value), widget->theme_group);
         break;
      case ETK_WIDGET_THEME_GROUP_PROPERTY:
         etk_widget_theme_set(widget, widget->theme_file, etk_property_value_string_get(value));
         break;
      case ETK_WIDGET_VISIBLE_PROPERTY:
         if (etk_property_value_bool_get(value))
            etk_widget_show(widget);
         else
            etk_widget_hide(widget);
         break;
      case ETK_WIDGET_VISIBILITY_LOCKED_PROPERTY:
         etk_widget_visibility_locked_set(widget, etk_property_value_bool_get(value));
         break;
      case ETK_WIDGET_WIDTH_REQUEST_PROPERTY:
         etk_widget_size_request_set(widget, etk_property_value_int_get(value), widget->requested_size.h);
         break;
      case ETK_WIDGET_HEIGHT_REQUEST_PROPERTY:
         etk_widget_size_request_set(widget, widget->requested_size.w, etk_property_value_int_get(value));
         break;
      case ETK_WIDGET_NAME_PROPERTY:
         etk_widget_name_set(widget, etk_property_value_string_get(value));
         break;
      case ETK_WIDGET_PARENT_PROPERTY:
         etk_widget_reparent(widget, ETK_CONTAINER(etk_property_value_pointer_get(value)));
         break;
      case ETK_WIDGET_REPEAT_EVENTS_PROPERTY:
         etk_widget_repeat_events_set(widget, etk_property_value_bool_get(value));
         break;
      case ETK_WIDGET_PASS_EVENTS_PROPERTY:
         etk_widget_pass_events_set(widget, etk_property_value_bool_get(value));
         break;
      case ETK_WIDGET_FOCUSABLE_PROPERTY:
         widget->focusable = etk_property_value_bool_get(value);
         etk_object_notify(object, "focusable");
      case ETK_WIDGET_FOCUS_ON_PRESS_PROPERTY:
         widget->focus_on_press = etk_property_value_bool_get(value);
         etk_object_notify(object, "focus_on_press");
         break;
      case ETK_WIDGET_CAN_PASS_FOCUS_PROPERTY:
         widget->can_pass_focus = etk_property_value_bool_get(value);
         etk_object_notify(object, "can_pass_focus");
         break;
      default:
         break;
   }
}

/* Gets the value of the property whose id is "property_id" */
static void _etk_widget_property_get(Etk_Object *object, int property_id, Etk_Property_Value *value)
{
   Etk_Widget *widget;

   if (!(widget = ETK_WIDGET(object)) || !value)
      return;

   switch (property_id)
   {
      case ETK_WIDGET_NAME_PROPERTY:
         etk_property_value_string_set(value, widget->name);
         break;
      case ETK_WIDGET_PARENT_PROPERTY:
         etk_property_value_pointer_set(value, ETK_OBJECT(widget->parent));
         break;
      case ETK_WIDGET_THEME_FILE_PROPERTY:
         etk_property_value_string_set(value, widget->theme_file);
         break;
      case ETK_WIDGET_THEME_GROUP_PROPERTY:
         etk_property_value_string_set(value, widget->theme_group);
         break;
      case ETK_WIDGET_WIDTH_REQUEST_PROPERTY:
         etk_property_value_int_set(value, widget->requested_size.w);
         break;
      case ETK_WIDGET_HEIGHT_REQUEST_PROPERTY:
         etk_property_value_int_set(value, widget->requested_size.h);
         break;
      case ETK_WIDGET_VISIBLE_PROPERTY:
         etk_property_value_bool_set(value, widget->visible);
         break;
      case ETK_WIDGET_VISIBILITY_LOCKED_PROPERTY:
         etk_property_value_bool_set(value, widget->visibility_locked);
         break;
      case ETK_WIDGET_REPEAT_EVENTS_PROPERTY:
         etk_property_value_bool_set(value, widget->repeat_events);
         break;
      case ETK_WIDGET_PASS_EVENTS_PROPERTY:
         etk_property_value_bool_set(value, widget->pass_events);
         break;
      case ETK_WIDGET_FOCUSABLE_PROPERTY:
         etk_property_value_bool_set(value, widget->focusable);
         break;
      case ETK_WIDGET_FOCUS_ON_PRESS_PROPERTY:
         etk_property_value_bool_set(value, widget->focus_on_press);
         break;
      case ETK_WIDGET_CAN_PASS_FOCUS_PROPERTY:
         etk_property_value_bool_set(value, widget->can_pass_focus);
         break;
      default:
         break;
   }
}

/**************************
 *
 * Callbacks and handlers
 *
 **************************/

/* Default handler for the "enter" signal */
static void _etk_widget_enter_handler(Etk_Widget *widget)
{
   if (!widget)
      return;
   etk_widget_theme_object_signal_emit(widget, "enter");
}

/* Default handler for the "leave" signal */
static void _etk_widget_leave_handler(Etk_Widget *widget)
{
   if (!widget)
      return;
   etk_widget_theme_object_signal_emit(widget, "leave");
}

/* Default handler for the "focus" signal */
static void _etk_widget_focus_handler(Etk_Widget *widget)
{
   if (!widget)
      return;
   etk_widget_theme_object_signal_emit(widget, "focus");
}

/* Default handler for the "unfocus" signal */
static void _etk_widget_unfocus_handler(Etk_Widget *widget)
{
   if (!widget)
      return;
   etk_widget_theme_object_signal_emit(widget, "unfocus");
}

/* Default handler for the "drag_drop" signal */
static void _etk_widget_drag_drop_handler(Etk_Widget *widget)
{
   if (!widget)
     return;
   etk_widget_theme_object_signal_emit(widget, "drag_drop");
}

/* Default handler for the "drag_motion" signal */
static void _etk_widget_drag_motion_handler(Etk_Widget *widget)
{
   if (!widget)
     return;
   etk_widget_theme_object_signal_emit(widget, "drag_motion");
}

/* Default handler for the "drag_leave" signal */
static void _etk_widget_drag_leave_handler(Etk_Widget *widget)
{
   if (!widget)
     return;
   etk_widget_theme_object_signal_emit(widget, "drag_leave");
}

/* Sets the widget as visible and queues a visibility update */
static void _etk_widget_show_handler(Etk_Widget *widget)
{
   if (!widget)
      return;
   etk_widget_theme_object_signal_emit(widget, "show");
}

/* Called when the mouse pointer enters the widget */
static void _etk_widget_mouse_in_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   Etk_Widget *widget;
   Evas_Event_Mouse_In *evas_event = event_info;
   Etk_Event_Mouse_In_Out event;

   if (!(widget = ETK_WIDGET(data)))
      return;
   
   if (object)
      _etk_widget_propagate_event = ETK_TRUE;
   
   event.buttons = evas_event->buttons;
   event.canvas.x = evas_event->canvas.x;
   event.canvas.y = evas_event->canvas.y;
   event.widget.x = evas_event->canvas.x - widget->inner_geometry.x;
   event.widget.y = evas_event->canvas.y - widget->inner_geometry.y;
   event.modifiers = evas_event->modifiers;
   event.locks = evas_event->locks;
   event.timestamp = evas_event->timestamp;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_MOUSE_IN_SIGNAL], ETK_OBJECT(widget), NULL, &event);
   
   if (_etk_widget_propagate_event && widget->parent)
      _etk_widget_mouse_in_cb(widget->parent, evas, NULL, event_info);
}

/* Called when the signal "mouse_in" is emitted */
static void _etk_widget_signal_mouse_in_cb(Etk_Object *object, Etk_Event_Mouse_In_Out *event, void *data)
{
   if (!object)
      return;
   etk_widget_enter(ETK_WIDGET(object));
}

/* Called when the mouse pointer leaves the widget */
static void _etk_widget_mouse_out_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   Etk_Widget *widget;
   Evas_Event_Mouse_Out *evas_event = event_info;
   Etk_Event_Mouse_In_Out event;

   if (!(widget = ETK_WIDGET(data)))
      return;

   if (object)
      _etk_widget_propagate_event = ETK_TRUE;
   
   event.buttons = evas_event->buttons;
   event.canvas.x = evas_event->canvas.x;
   event.canvas.y = evas_event->canvas.y;
   event.widget.x = evas_event->canvas.x - widget->inner_geometry.x;
   event.widget.y = evas_event->canvas.y - widget->inner_geometry.y;
   event.modifiers = evas_event->modifiers;
   event.locks = evas_event->locks;
   event.timestamp = evas_event->timestamp;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_MOUSE_OUT_SIGNAL], ETK_OBJECT(widget), NULL, &event);
   
   if (_etk_widget_propagate_event && widget->parent)
      _etk_widget_mouse_out_cb(widget->parent, evas, NULL, event_info);
}

/* Called when the signal "mouse_out" is emitted */
static void _etk_widget_signal_mouse_out_cb(Etk_Object *object, Etk_Event_Mouse_In_Out *event, void *data)
{
   if (!object)
      return;
   etk_widget_leave(ETK_WIDGET(object));
}

/* Called when the mouse pointer moves */
static void _etk_widget_mouse_move_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   Etk_Widget *widget;
   Evas_Event_Mouse_Move *evas_event = event_info;
   Etk_Event_Mouse_Move event;

   if (!(widget = ETK_WIDGET(data)))
      return;

   if (object)
      _etk_widget_propagate_event = ETK_TRUE;
   
   event.buttons = evas_event->buttons;
   event.cur.canvas.x = evas_event->cur.canvas.x;
   event.cur.canvas.y = evas_event->cur.canvas.y;
   event.cur.widget.x = evas_event->cur.canvas.x - widget->inner_geometry.x;
   event.cur.widget.y = evas_event->cur.canvas.y - widget->inner_geometry.y;
   event.prev.canvas.x = evas_event->prev.canvas.x;
   event.prev.canvas.y = evas_event->prev.canvas.y;
   event.prev.widget.x = evas_event->prev.canvas.x - widget->inner_geometry.x;
   event.prev.widget.y = evas_event->prev.canvas.y - widget->inner_geometry.y;
   event.modifiers = evas_event->modifiers;
   event.locks = evas_event->locks;
   event.timestamp = evas_event->timestamp;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_MOUSE_MOVE_SIGNAL], ETK_OBJECT(widget), NULL, &event);
   
   if (_etk_widget_propagate_event && widget->parent)
      _etk_widget_mouse_move_cb(widget->parent, evas, NULL, event_info);
}

/* Called when the mouse presses the widget */
static void _etk_widget_mouse_down_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   Etk_Widget *widget;
   Evas_Event_Mouse_Down *evas_event = event_info;
   Etk_Event_Mouse_Up_Down event;

   if (!(widget = ETK_WIDGET(data)))
      return;
   
   if (object)
      _etk_widget_propagate_event = ETK_TRUE;
   
   event.button = evas_event->button;
   event.canvas.x = evas_event->canvas.x;
   event.canvas.y = evas_event->canvas.y;
   event.widget.x = evas_event->canvas.x - widget->inner_geometry.x;
   event.widget.y = evas_event->canvas.y - widget->inner_geometry.y;
   event.modifiers = evas_event->modifiers;
   event.locks = evas_event->locks;
   event.flags = evas_event->flags;
   event.timestamp = evas_event->timestamp;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_MOUSE_DOWN_SIGNAL], ETK_OBJECT(widget), NULL, &event);
   
   if (_etk_widget_propagate_event && widget->repeat_events && widget->parent)
      _etk_widget_mouse_down_cb(widget->parent, evas, NULL, event_info);
}

/* Called when the widget is pressed */
static void _etk_widget_signal_mouse_down_cb(Etk_Object *object, Etk_Event_Mouse_Up_Down *event, void *data)
{
   Etk_Widget *widget;
 
   if (!(widget = ETK_WIDGET(object)) || !widget->focus_on_press)
      return;
   etk_widget_focus(widget);
}

/* Called when the mouse releases the widget */
static void _etk_widget_mouse_up_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   Etk_Widget *widget;
   Evas_Event_Mouse_Up *evas_event = event_info;
   Etk_Event_Mouse_Up_Down event;

   if (!(widget = ETK_WIDGET(data)))
      return;

   if (object)
      _etk_widget_propagate_event = ETK_TRUE;
   
   event.button = evas_event->button;
   event.canvas.x = evas_event->canvas.x;
   event.canvas.y = evas_event->canvas.y;
   event.widget.x = evas_event->canvas.x - widget->inner_geometry.x;
   event.widget.y = evas_event->canvas.y - widget->inner_geometry.y;
   event.modifiers = evas_event->modifiers;
   event.locks = evas_event->locks;
   event.flags = evas_event->flags;
   event.timestamp = evas_event->timestamp;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_MOUSE_UP_SIGNAL], ETK_OBJECT(widget), NULL, &event);

   if (evas_event->canvas.x >= widget->geometry.x && evas_event->canvas.x <= widget->geometry.x + widget->geometry.w &&
         evas_event->canvas.y >= widget->geometry.y && evas_event->canvas.y <= widget->geometry.y + widget->geometry.h)
      etk_signal_emit(_etk_widget_signals[ETK_WIDGET_MOUSE_CLICK_SIGNAL], ETK_OBJECT(widget), NULL, &event);
   
   if (_etk_widget_propagate_event && widget->repeat_events && widget->parent)
      _etk_widget_mouse_up_cb(widget->parent, evas, NULL, event_info);
}

/* Called when the mouse wheel is used over the widget */
static void _etk_widget_mouse_wheel_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   Etk_Widget *widget;
   Evas_Event_Mouse_Wheel *evas_event = event_info;
   Etk_Event_Mouse_Wheel event;

   if (!(widget = ETK_WIDGET(data)))
      return;
   
   if (object)
      _etk_widget_propagate_event = ETK_TRUE;
   
   event.direction = evas_event->direction;
   event.z = evas_event->z;
   event.canvas.x = evas_event->canvas.x;
   event.canvas.y = evas_event->canvas.y;
   event.widget.x = evas_event->canvas.x - widget->inner_geometry.x;
   event.widget.y = evas_event->canvas.y - widget->inner_geometry.y;
   event.modifiers = evas_event->modifiers;
   event.locks = evas_event->locks;
   event.timestamp = evas_event->timestamp;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_MOUSE_WHEEL_SIGNAL], ETK_OBJECT(widget), NULL, &event);
   
   if (_etk_widget_propagate_event && widget->parent && ETK_WIDGET(widget->parent)->smart_object)
      _etk_widget_mouse_wheel_cb(widget->parent, evas, NULL, event_info);
}

/* Called when the user presses a key and if the widget is focused */
static void _etk_widget_key_down_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   Etk_Widget *widget;
   Evas_Event_Key_Down *evas_event = event_info;
   Etk_Event_Key_Up_Down event;

   if (!(widget = ETK_WIDGET(data)))
      return;

   if (object)
      _etk_widget_propagate_event = ETK_TRUE;
   
   event.keyname = evas_event->keyname;
   event.modifiers = evas_event->modifiers;
   event.locks = evas_event->locks;
   event.key = evas_event->key;
   event.string = evas_event->string;
   event.compose = evas_event->compose;
   event.timestamp = evas_event->timestamp;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_KEY_DOWN_SIGNAL], ETK_OBJECT(widget), NULL, &event);
   
   if (_etk_widget_propagate_event && widget->parent && ETK_WIDGET(widget->parent)->smart_object)
      _etk_widget_key_down_cb(widget->parent, evas, NULL, event_info);
}

/* Called when a key is pressed */
static void _etk_widget_signal_key_down_cb(Etk_Object *object, Etk_Event_Key_Up_Down *event, void *data)
{
   Etk_Widget *widget;
   Etk_Toplevel_Widget *toplevel;

   if (!(widget = ETK_WIDGET(object)) || !event || !(toplevel = (widget->toplevel_parent)) || !event->key)
      return;

   if (object)
      _etk_widget_propagate_event = ETK_TRUE;

   if (strcmp(event->key, "Tab") == 0)
   {
      if (widget->can_pass_focus)
         etk_widget_focus(etk_toplevel_widget_focused_widget_next_get(toplevel));
      etk_widget_event_propagation_stop();
   }
   else if (strcmp(event->key, "ISO_Left_Tab") == 0)
   {
      if (widget->can_pass_focus)
         etk_widget_focus(etk_toplevel_widget_focused_widget_prev_get(toplevel));
      etk_widget_event_propagation_stop();
   }
}

/* Called when the user releases a key and if the widget is focused */
static void _etk_widget_key_up_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   Etk_Widget *widget;
   Evas_Event_Key_Up *evas_event = event_info;
   Etk_Event_Key_Up_Down event;

   if (!(widget = ETK_WIDGET(data)))
      return;

   if (object)
      _etk_widget_propagate_event = ETK_TRUE;
   
   event.keyname = evas_event->keyname;
   event.modifiers = evas_event->modifiers;
   event.locks = evas_event->locks;
   event.key = evas_event->key;
   event.string = evas_event->string;
   event.compose = evas_event->compose;
   event.timestamp = evas_event->timestamp;

   etk_signal_emit(_etk_widget_signals[ETK_WIDGET_KEY_UP_SIGNAL], ETK_OBJECT(widget), NULL, &event);
   
   if (_etk_widget_propagate_event && widget->parent && ETK_WIDGET(widget->parent)->smart_object)
      _etk_widget_key_up_cb(widget->parent, evas, NULL, event_info);
}

#if HAVE_ECORE_X
void etk_widget_xdnd_set(Etk_Widget *widget, Etk_Bool on)
{
   if(on)
     {
	widget->accepts_xdnd = 1;
	_etk_dnd_widgets = evas_list_append(_etk_dnd_widgets, widget);
     }
   else
     {
	widget->accepts_xdnd = 0;
	_etk_dnd_widgets = evas_list_remove(_etk_dnd_widgets, widget);
     }
}

Etk_Bool etk_widget_xdnd_get(Etk_Widget *widget)
{
   if(widget->accepts_xdnd) return ETK_TRUE;

   return ETK_FALSE;
}

const char **etk_widget_xdnd_files_get(Etk_Widget *widget, int *num_files)
{
   if(!widget->accepts_xdnd || widget->xdnd_files == NULL)
     return NULL;
   
   if(num_files)
     *num_files = widget->xdnd_files_num;
     
   return (const char **)widget->xdnd_files;
}
#endif


/**************************
 *
 * Private functions
 *
 **************************/

/* Used by etk_widget_parent_set */
static void _etk_widget_toplevel_parent_set(Etk_Widget *widget, void *data)
{
   if (!widget)
      return;

   widget->toplevel_parent = ETK_TOPLEVEL_WIDGET(data);
   if (ETK_IS_CONTAINER(widget))
      etk_container_for_each_data(ETK_CONTAINER(widget), _etk_widget_toplevel_parent_set, widget->toplevel_parent);
}

/* Realizes the widget and all its children */
static void _etk_widget_realize_all(Etk_Widget *widget)
{
   if (!widget)
      return;

   etk_widget_realize(widget);
   if (ETK_IS_CONTAINER(widget))
      etk_container_for_each(ETK_CONTAINER(widget), _etk_widget_realize_all);
}

/* Unrealizes the widget and all its children */
static void _etk_widget_unrealize_all(Etk_Widget *widget)
{
   if (!widget)
      return;

   etk_widget_unrealize(widget);
   if (ETK_IS_CONTAINER(widget))
      etk_container_for_each(ETK_CONTAINER(widget), _etk_widget_unrealize_all);
}

/* Used by etk_widget_redraw_queue() */
static void _etk_widget_redraw_queue_recursive(Etk_Widget *widget)
{
   if (!widget)
      return;

   widget->need_redraw = ETK_TRUE;
   if (ETK_IS_CONTAINER(widget))
      etk_container_for_each(ETK_CONTAINER(widget), _etk_widget_redraw_queue_recursive);
}

/* Makes the theme object of "swallowing_widget" swallow "object" in a part called "part" */
Etk_Bool _etk_widget_theme_object_swallow_full(Etk_Widget *swallowing_widget, const char *part, Evas_Object *object, Etk_Bool is_widget_theme_object)
{
   Evas *evas;
   Evas_Object *previously_swallowed;
   Etk_Widget_Swallowed_Object *swallowed_object;

   if (!swallowing_widget || !part || !object || !swallowing_widget->theme_object)
      return ETK_FALSE;
   if (!(evas = etk_widget_toplevel_evas_get(swallowing_widget)) || (evas != evas_object_evas_get(object)))
      return ETK_FALSE;
   if (!edje_object_part_exists(swallowing_widget->theme_object, part))
      return ETK_FALSE;

   if ((previously_swallowed = edje_object_part_swallow_get(swallowing_widget->theme_object, part)))
      etk_widget_theme_object_unswallow(swallowing_widget, previously_swallowed);
   etk_widget_member_object_del(swallowing_widget, object);

   swallowed_object = malloc(sizeof(Etk_Widget_Swallowed_Object));
   swallowed_object->object = object;
   swallowed_object->swallowing_part = strdup(part);
   swallowed_object->is_widget_theme_object = is_widget_theme_object;
   swallowing_widget->swallowed_objects = evas_list_append(swallowing_widget->swallowed_objects, swallowed_object);
   edje_object_part_swallow(swallowing_widget->theme_object, part, object);
   etk_widget_size_recalc_queue(swallowing_widget);

   return ETK_TRUE;
}

/* Finds if an evas object is a member object of the widget */
static Evas_List *_etk_widget_member_object_find(Etk_Widget *widget, Evas_Object *object)
{
   Evas_List *l;
   Etk_Widget_Member_Object *m;
   
   if (!widget || !object)
      return NULL;
   
   for (l = widget->member_objects; l; l = l->next)
   {
      m = l->data;
      if (m->object == object)
         return l;
   }
   return NULL;
}

/* Called when a member object of the widget requests to be shown. Shows it only if all the parents are visible.
 Should not be necessary with futute releases of evas */
static void _etk_widget_member_object_intercept_show_cb(void *data, Evas_Object *obj)
{
   Etk_Widget *last_visible_parent;
   Evas_List *l;
   Etk_Widget_Member_Object *member_object;
   
   if (!(last_visible_parent = ETK_WIDGET(data)))
      return;
   
   if ((l = _etk_widget_member_object_find(last_visible_parent, obj)))
   {
      member_object = l->data;
      member_object->visible = ETK_TRUE;
   }
   
   while (last_visible_parent->parent && ETK_WIDGET(last_visible_parent->parent)->visible)
      last_visible_parent = ETK_WIDGET(last_visible_parent->parent);
   if (ETK_IS_TOPLEVEL_WIDGET(last_visible_parent))
      evas_object_show(obj);
}


/* Called when a member object of the widget requests to be hidden.
 Should not be necessary with futute releases of evas */
static void _etk_widget_member_object_intercept_hide_cb(void *data, Evas_Object *obj)
{
   Evas_List *l;
   Etk_Widget *widget;
   Etk_Widget_Member_Object *member_object;
   
   if (!(widget = ETK_WIDGET(data)))
      return;
   
   if ((l = _etk_widget_member_object_find(widget, obj)))
   {
      member_object = l->data;
      member_object->visible = ETK_FALSE;
   }
   evas_object_hide(obj);
}

/* Creates a new object to swallow */
static Evas_Object *_etk_widget_smart_object_add(Evas *evas, Etk_Widget *widget)
{
   Evas_Object *new_object;
   Etk_Widget_Smart_Data *smart_data;

   if (!_etk_widget_smart_object_smart)
   {
      _etk_widget_smart_object_smart = evas_smart_new("etk_widget_swallow_object",
         NULL, /* add */
         _etk_widget_smart_object_del, /* del */
         NULL, /* layer_set */
         NULL, /* raise */
         NULL, /* lower */
         NULL, /* stack_above */
         NULL, /* stack_below */
         _etk_widget_smart_object_move, /* move */
         _etk_widget_smart_object_resize, /* resize */
         _etk_widget_smart_object_show, /* show */
         _etk_widget_smart_object_hide, /* hide */
         NULL, /* color_set */
         _etk_widget_smart_object_clip_set, /* clip_set */
         _etk_widget_smart_object_clip_unset, /* clip_unset */
         NULL); /* data*/
   }

   new_object = evas_object_smart_add(evas, _etk_widget_smart_object_smart);
   smart_data = calloc(1, sizeof(Etk_Widget_Smart_Data));
   smart_data->widget = widget;
   evas_object_smart_data_set(new_object, smart_data);
   evas_object_data_set(new_object, "etk_widget", widget);
   return new_object;
}

/* Called when the smart object is deleted */
static void _etk_widget_smart_object_del(Evas_Object *object)
{
   Etk_Widget_Smart_Data *smart_data;

   if (!object || !(smart_data = evas_object_smart_data_get(object)))
      return;

   free(smart_data);
}

/* Called when the smart object is moved */
static void _etk_widget_smart_object_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   Etk_Widget *widget;
   Etk_Widget_Smart_Data *smart_data;

   if (!object || !(smart_data = evas_object_smart_data_get(object)) || !(widget = smart_data->widget))
      return;

   if (x != widget->geometry.x || y != widget->geometry.y)
   {
      Evas_List *l;
      Etk_Widget_Member_Object *m;
      Evas_Coord child_x, child_y;
      int x_offset, y_offset;

      x_offset = x - widget->geometry.x;
      y_offset = y - widget->geometry.y;
      widget->geometry.x = x;
      widget->geometry.y = y;
      widget->inner_geometry.x = widget->geometry.x + widget->left_inset;
      widget->inner_geometry.y = widget->geometry.y + widget->top_inset;

      for (l = widget->member_objects; l; l = l->next)
      {
         m = l->data;
         evas_object_geometry_get(m->object, &child_x, &child_y, NULL, NULL);
         evas_object_move(m->object, child_x + x_offset, child_y + y_offset);
      }
   }
}

/* Called when the smart object is resized */
static void _etk_widget_smart_object_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   Etk_Widget *widget;
   Etk_Widget_Smart_Data *smart_data;
      
   if (!object || !(smart_data = evas_object_smart_data_get(object)) || !(widget = smart_data->widget))
      return;

   if (w != widget->geometry.w || h != widget->geometry.h || widget->need_redraw)
   {
      widget->geometry.w = w;
      widget->geometry.h = h;
      widget->inner_geometry.w = widget->geometry.w - widget->left_inset - widget->right_inset;
      widget->inner_geometry.h = widget->geometry.h - widget->top_inset - widget->bottom_inset;

      if (widget->theme_object)
      {
         evas_object_move(widget->theme_object, widget->geometry.x, widget->geometry.y);
         evas_object_resize(widget->theme_object, widget->geometry.w, widget->geometry.h);
      }
      if (widget->size_allocate)	
	widget->size_allocate(widget, widget->inner_geometry);	
      
      widget->need_redraw = ETK_FALSE;
   }
}

/* Called when the smart object is shown */
static void _etk_widget_smart_object_show(Evas_Object *object)
{
   Evas_List *l;
   Etk_Widget_Member_Object *m;
   Etk_Widget_Smart_Data *smart_data;
   Etk_Widget *widget;
   
   if (!object || !(smart_data = evas_object_smart_data_get(object)) || !(widget = smart_data->widget))
      return;
   
   for (l = widget->member_objects; l; l = l->next)
   {
      m = l->data;
      if (m->visible)
         evas_object_show(m->object);
   }
}

/* Called when the smart object is hidden */
static void _etk_widget_smart_object_hide(Evas_Object *object)
{
   Evas_List *l;
   Etk_Widget_Member_Object *m;
   Etk_Widget_Smart_Data *smart_data;
   Etk_Widget *widget;
   Etk_Bool is_visible;

   if (!object || !(smart_data = evas_object_smart_data_get(object)) || !(widget = smart_data->widget))
      return;
   
   for (l = widget->member_objects; l; l = l->next)
   {
      m = l->data;
      is_visible = m->visible;
      evas_object_hide(m->object);
      m->visible = is_visible;
   }
}

/* Called when a clip is set to the smart object */
static void _etk_widget_smart_object_clip_set(Evas_Object *object, Evas_Object *clip)
{
   Evas_List *l;
   Etk_Widget_Member_Object *m;
   Etk_Widget_Smart_Data *smart_data;
   Etk_Widget *widget, *child;
   Evas_Object *object_to_clip;

   if (!object || !clip || !(smart_data = evas_object_smart_data_get(object)) || !(widget = smart_data->widget))
      return;

   if (widget->clip)
      etk_widget_clip_unset(widget);
   widget->clip = clip;

   for (l = widget->member_objects; l; l = l->next)
   {
      m = l->data;
      if (!(child = ETK_WIDGET(evas_object_data_get(m->object, "etk_widget"))) || !child->clip)
      {
         object_to_clip = m->object;
         while (evas_object_clip_get(object_to_clip))
         {
            if (object_to_clip == clip)
               break;
            object_to_clip = evas_object_clip_get(object_to_clip);
         }
         if (object_to_clip != clip)
            evas_object_clip_set(object_to_clip, clip);
      }
      else
         evas_object_clip_set(child->clip, clip);
   }
}

/* Called when the clip of the smart object is unset */
static void _etk_widget_smart_object_clip_unset(Evas_Object *object)
{
   Evas_List *l;
   Etk_Widget_Member_Object *m;
   Etk_Widget *widget, *child;
   Etk_Widget_Smart_Data *smart_data;

   if (!object || !(smart_data = evas_object_smart_data_get(object)) || !(widget = smart_data->widget) || !widget->clip)
      return;

   for (l = widget->member_objects; l; l = l->next)
   {
      m = l->data;
      if (!(child = ETK_WIDGET(evas_object_data_get(m->object, "etk_widget"))))
      {
         if (evas_object_clip_get(m->object) == widget->clip)
            evas_object_clip_unset(m->object);
      }
      else if (child->clip)
      {
         if (child->clip == widget->clip)
            etk_widget_clip_unset(child);
         else
         {
            Evas_Object *c;

            for (c = child->clip; c; c = evas_object_clip_get(c))
            {
               if (evas_object_clip_get(c) == widget->clip)
               {
                  evas_object_clip_unset(c);
                  break;
               }
            }
         }
      }
   }
   widget->clip = NULL;
}

/* Called when the swallowed objet is removed from the list of swallowed objects of the widget */
static void _etk_widget_swallowed_object_free(void *data)
{
   Etk_Widget_Swallowed_Object *swallowed_object;

   if (!(swallowed_object = data))
      return;

   free(swallowed_object->swallowing_part);
   free(swallowed_object);
}

/** @} */
