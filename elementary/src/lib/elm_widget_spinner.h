#ifndef ELM_WIDGET_SPINNER_H
#define ELM_WIDGET_SPINNER_H

#include "Elementary.h"

#ifdef HAVE_EIO
# include <Eio.h>
#endif

/**
 * @addtogroup Widget
 * @{
 *
 * @section elm-spinner-class The Elementary Spinner Class
 *
 * Elementary, besides having the @ref Spinner widget, exposes its
 * foundation -- the Elementary Spinner Class -- in order to create other
 * widgets which are a spinner with some more logic on top.
 */

/**
 * Base layout smart data extended with spinner instance data.
 */
typedef struct _Elm_Spinner_Smart_Data    Elm_Spinner_Smart_Data;
struct _Elm_Spinner_Smart_Data
{
   Evas_Object          *ent;
   const char           *label;
   double                val, val_min, val_max, orig_val, step, val_base;
   double                drag_start_pos, spin_speed, interval, first_interval;
   int                   round;
   Ecore_Timer          *delay, *spin;
   Eina_List            *special_values;

   Eina_Bool             entry_visible : 1;
   Eina_Bool             dragging : 1;
   Eina_Bool             editable : 1;
   Eina_Bool             wrap : 1;
};

typedef struct _Elm_Spinner_Special_Value Elm_Spinner_Special_Value;
struct _Elm_Spinner_Special_Value
{
   double      value;
   const char *label;
};

/**
 * @}
 */

#define ELM_SPINNER_DATA_GET(o, sd) \
  Elm_Spinner_Smart_Data * sd = eo_data_get(o, ELM_OBJ_SPINNER_CLASS)

#define ELM_SPINNER_DATA_GET_OR_RETURN(o, ptr)       \
  ELM_SPINNER_DATA_GET(o, ptr);                      \
  if (!ptr)                                          \
    {                                                \
       CRITICAL("No widget data for object %p (%s)", \
                o, evas_object_type_get(o));         \
       return;                                       \
    }

#define ELM_SPINNER_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_SPINNER_DATA_GET(o, ptr);                         \
  if (!ptr)                                             \
    {                                                   \
       CRITICAL("No widget data for object %p (%s)",    \
                o, evas_object_type_get(o));            \
       return val;                                      \
    }

#define ELM_SPINNER_CHECK(obj)                     \
  if (!eo_isa((obj), ELM_OBJ_SPINNER_CLASS)) \
    return

#endif
