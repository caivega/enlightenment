#ifndef ELM_WIDGET_THUMB_H
#define ELM_WIDGET_THUMB_H

#include "Elementary.h"

/**
 * @addtogroup Widget
 * @{
 *
 * @section elm-thumb-class The Elementary Thumb Class
 *
 * Elementary, besides having the @ref Thumb widget, exposes its
 * foundation -- the Elementary Thumb Class -- in order to create
 * other widgets which are a thumb with some more logic on top.
 */

/**
 * Base widget smart data extended with thumb instance data.
 */
typedef struct _Elm_Thumb_Smart_Data Elm_Thumb_Smart_Data;
struct _Elm_Thumb_Smart_Data
{
   Evas_Object          *obj; // the object itself
   Evas_Object          *view;  /* actual thumbnail, to be swallowed
                                 * at the thumb frame */

   /* original object's file/key pair */
   const char           *file;
   const char           *key;

   struct
   {
      /* object's thumbnail file/key pair */
      const char          *file;
      const char          *key;
#ifdef HAVE_ELEMENTARY_ETHUMB
      const char          *thumb_path;
      const char          *thumb_key;
      Ethumb_Client_Async *request;

      Ethumb_Thumb_Format  format;

      Eina_Bool            retry : 1;
#endif
   } thumb;

   Ecore_Event_Handler        *eeh;
   Elm_Thumb_Animation_Setting anim_setting;

   Eina_Bool                   edit : 1;
   Eina_Bool                   on_hold : 1;
   Eina_Bool                   is_video : 1;
   Eina_Bool                   was_video : 1;
};

/**
 * @}
 */

#define ELM_THUMB_DATA_GET(o, sd) \
  Elm_Thumb_Smart_Data * sd = eo_data_get(o, ELM_OBJ_THUMB_CLASS)

#define ELM_THUMB_DATA_GET_OR_RETURN(o, ptr)         \
  ELM_THUMB_DATA_GET(o, ptr);                        \
  if (!ptr)                                          \
    {                                                \
       CRITICAL("No widget data for object %p (%s)", \
                o, evas_object_type_get(o));         \
       return;                                       \
    }

#define ELM_THUMB_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_THUMB_DATA_GET(o, ptr);                         \
  if (!ptr)                                           \
    {                                                 \
       CRITICAL("No widget data for object %p (%s)",  \
                o, evas_object_type_get(o));          \
       return val;                                    \
    }

#define ELM_THUMB_CHECK(obj)                                                 \
  if (!eo_isa((obj), ELM_OBJ_THUMB_CLASS))      \
    return

#endif
