#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_layout.h"

static const char PROGRESSBAR_SMART_NAME[] = "elm_progressbar";

#define MIN_RATIO_LVL 0.0
#define MAX_RATIO_LVL 1.0

typedef struct _Elm_Progressbar_Smart_Data Elm_Progressbar_Smart_Data;

struct _Elm_Progressbar_Smart_Data
{
   Elm_Layout_Smart_Data base;

   Evas_Object          *spacer;
   const char           *units;

   Evas_Coord            size;
   double                val;

   Eina_Bool             horizontal : 1;
   Eina_Bool             inverted : 1;
   Eina_Bool             pulse : 1;
   Eina_Bool             pulse_state : 1;
};

#define ELM_PROGRESSBAR_DATA_GET(o, sd) \
  Elm_Progressbar_Smart_Data * sd = evas_object_smart_data_get(o)

#define ELM_PROGRESSBAR_DATA_GET_OR_RETURN(o, ptr)   \
  ELM_PROGRESSBAR_DATA_GET(o, ptr);                  \
  if (!ptr)                                          \
    {                                                \
       CRITICAL("No widget data for object %p (%s)", \
                o, evas_object_type_get(o));         \
       return;                                       \
    }

#define ELM_PROGRESSBAR_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_PROGRESSBAR_DATA_GET(o, ptr);                         \
  if (!ptr)                                                 \
    {                                                       \
       CRITICAL("No widget data for object %p (%s)",        \
                o, evas_object_type_get(o));                \
       return val;                                          \
    }

#define ELM_PROGRESSBAR_CHECK(obj)                 \
  if (!obj || !elm_widget_type_check               \
        ((obj), PROGRESSBAR_SMART_NAME, __func__)) \
    return

/* Inheriting from elm_layout. Besides, we need no more than what is
 * there */
EVAS_SMART_SUBCLASS_NEW
  (PROGRESSBAR_SMART_NAME, _elm_progressbar, Elm_Layout_Smart_Class,
  Elm_Layout_Smart_Class, elm_layout_smart_class_get, NULL);

static const Elm_Layout_Part_Alias_Description _content_aliases[] =
{
   {"icon", "elm.swallow.content"},
   {NULL, NULL}
};

static const Elm_Layout_Part_Alias_Description _text_aliases[] =
{
   {"default", "elm.text"},
   {NULL, NULL}
};

static void
_units_set(Evas_Object *obj)
{
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   if (sd->units)
     {
        char buf[1024];

        snprintf(buf, sizeof(buf), sd->units, 100 * sd->val);
        elm_layout_text_set(obj, "elm.text.status", buf);
     }
   else elm_layout_text_set(obj, "elm.text.status", NULL);
}

static void
_val_set(Evas_Object *obj)
{
   Eina_Bool rtl;
   double pos;

   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   pos = sd->val;
   rtl = elm_widget_mirrored_get(obj);

   if ((!rtl && sd->inverted) ||
       (rtl && ((!sd->horizontal && sd->inverted) ||
                (sd->horizontal && !sd->inverted))))
     pos = MAX_RATIO_LVL - pos;

   edje_object_part_drag_value_set
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm.cur.progressbar", pos, pos);
}

static void
_elm_progressbar_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;

   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   edje_object_size_min_restricted_calc
     (ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

/* FIXME: replicated from elm_layout just because progressbar's icon
 * spot is elm.swallow.content, not elm.swallow.icon. Fix that
 * whenever we can changed the theme API */
static void
_icon_signal_emit(Evas_Object *obj)
{
   char buf[64];

   snprintf(buf, sizeof(buf), "elm,state,icon,%s",
            elm_layout_content_get(obj, "icon") ? "visible" : "hidden");

   elm_layout_signal_emit(obj, buf, "elm");
}

/* FIXME: replicated from elm_layout just because progressbar's icon
 * spot is elm.swallow.content, not elm.swallow.icon. Fix that
 * whenever we can changed the theme API */
static Eina_Bool
_elm_progressbar_smart_sub_object_del(Evas_Object *obj,
                                      Evas_Object *sobj)
{
   if (!ELM_WIDGET_CLASS(_elm_progressbar_parent_sc)->sub_object_del
         (obj, sobj))
     return EINA_FALSE;

   _icon_signal_emit(obj);

   return EINA_TRUE;
}

/* FIXME: replicated from elm_layout just because progressbar's icon
 * spot is elm.swallow.content, not elm.swallow.icon. Fix that
 * whenever we can changed the theme API */
static Eina_Bool
_elm_progressbar_smart_content_set(Evas_Object *obj,
                                   const char *part,
                                   Evas_Object *content)
{
   if (!ELM_CONTAINER_CLASS(_elm_progressbar_parent_sc)->content_set
         (obj, part, content))
     return EINA_FALSE;

   _icon_signal_emit(obj);

   return EINA_TRUE;
}

static Eina_Bool
_elm_progressbar_smart_theme(Evas_Object *obj)
{
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   if (sd->horizontal)
     eina_stringshare_replace(&ELM_LAYOUT_DATA(sd)->group, "horizontal");
   else eina_stringshare_replace(&ELM_LAYOUT_DATA(sd)->group, "vertical");

   if (!ELM_WIDGET_CLASS(_elm_progressbar_parent_sc)->theme(obj))
     return EINA_FALSE;

   if (sd->pulse)
     elm_layout_signal_emit(obj, "elm,state,pulse", "elm");
   else
     elm_layout_signal_emit(obj, "elm,state,fraction", "elm");

   if (sd->pulse_state)
     elm_layout_signal_emit(obj, "elm,state,pulse,start", "elm");

   if ((sd->units) && (!sd->pulse))
     elm_layout_signal_emit(obj, "elm,state,units,visible", "elm");

   if (sd->horizontal)
     evas_object_size_hint_min_set
       (sd->spacer, (double)sd->size * elm_widget_scale_get(obj) *
       elm_config_scale_get(), 1);
   else
     evas_object_size_hint_min_set
       (sd->spacer, 1, (double)sd->size * elm_widget_scale_get(obj) *
       elm_config_scale_get());

   if (sd->inverted)
     elm_layout_signal_emit(obj, "elm,state,inverted,on", "elm");

   _units_set(obj);
   _val_set(obj);

   /* FIXME: replicated from elm_layout just because progressbar's
    * icon spot is elm.swallow.content, not elm.swallow.icon. Fix that
    * whenever we can changed the theme API */
   _icon_signal_emit(obj);

   edje_object_message_signal_process(ELM_WIDGET_DATA(sd)->resize_obj);

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static void
_elm_progressbar_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Progressbar_Smart_Data);

   ELM_WIDGET_CLASS(_elm_progressbar_parent_sc)->base.add(obj);

   priv->horizontal = EINA_TRUE;
   priv->inverted = EINA_FALSE;
   priv->pulse = EINA_FALSE;
   priv->pulse_state = EINA_FALSE;
   priv->units = eina_stringshare_add("%.0f %%");
   priv->val = MIN_RATIO_LVL;

   elm_layout_theme_set
     (obj, "progressbar", "horizontal", elm_widget_style_get(obj));

   priv->spacer = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(priv->spacer, 0, 0, 0, 0);
   evas_object_pass_events_set(priv->spacer, EINA_TRUE);

   elm_layout_content_set(obj, "elm.swallow.bar", priv->spacer);

   _units_set(obj);
   _val_set(obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);

   elm_layout_sizing_eval(obj);
}

static void
_elm_progressbar_smart_del(Evas_Object *obj)
{
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   if (sd->units) eina_stringshare_del(sd->units);

   ELM_WIDGET_CLASS(_elm_progressbar_parent_sc)->base.del(obj);
}

static void
_elm_progressbar_smart_set_user(Elm_Layout_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_progressbar_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_progressbar_smart_del;

   ELM_WIDGET_CLASS(sc)->theme = _elm_progressbar_smart_theme;
   ELM_WIDGET_CLASS(sc)->sub_object_del =
     _elm_progressbar_smart_sub_object_del;

   /* not a 'focus chain manager' */
   ELM_WIDGET_CLASS(sc)->focus_next = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction = NULL;

   ELM_CONTAINER_CLASS(sc)->content_set = _elm_progressbar_smart_content_set;

   sc->sizing_eval = _elm_progressbar_smart_sizing_eval;

   sc->content_aliases = _content_aliases;
   sc->text_aliases = _text_aliases;
}

EAPI Evas_Object *
elm_progressbar_add(Evas_Object *parent)
{
   Evas *e;
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   e = evas_object_evas_get(parent);
   if (!e) return NULL;

   obj = evas_object_smart_add(e, _elm_progressbar_smart_class_new());

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   return obj;
}

EAPI void
elm_progressbar_pulse_set(Evas_Object *obj,
                          Eina_Bool pulse)
{
   ELM_PROGRESSBAR_CHECK(obj);
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   pulse = !!pulse;
   if (sd->pulse == pulse) return;

   sd->pulse = pulse;

   _elm_progressbar_smart_theme(obj);
}

EAPI Eina_Bool
elm_progressbar_pulse_get(const Evas_Object *obj)
{
   ELM_PROGRESSBAR_CHECK(obj) EINA_FALSE;
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   return sd->pulse;
}

EAPI void
elm_progressbar_pulse(Evas_Object *obj,
                      Eina_Bool state)
{
   ELM_PROGRESSBAR_CHECK(obj);
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   state = !!state;
   if ((!sd->pulse) && (sd->pulse_state == state)) return;

   sd->pulse_state = state;

   if (sd->pulse_state)
     elm_layout_signal_emit(obj, "elm,state,pulse,start", "elm");
   else
     elm_layout_signal_emit(obj, "elm,state,pulse,stop", "elm");
}

EAPI void
elm_progressbar_value_set(Evas_Object *obj,
                          double val)
{
   ELM_PROGRESSBAR_CHECK(obj);
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   if (sd->val == val) return;

   sd->val = val;
   if (sd->val < MIN_RATIO_LVL) sd->val = MIN_RATIO_LVL;
   if (sd->val > MAX_RATIO_LVL) sd->val = MAX_RATIO_LVL;

   _val_set(obj);
   _units_set(obj);
}

EAPI double
elm_progressbar_value_get(const Evas_Object *obj)
{
   ELM_PROGRESSBAR_CHECK(obj) 0.0;
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   return sd->val;
}

EAPI void
elm_progressbar_span_size_set(Evas_Object *obj,
                              Evas_Coord size)
{
   ELM_PROGRESSBAR_CHECK(obj);
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   if (sd->size == size) return;

   sd->size = size;

   if (sd->horizontal)
     evas_object_size_hint_min_set
       (sd->spacer, (double)sd->size * elm_widget_scale_get(obj) *
       elm_config_scale_get(), 1);
   else
     evas_object_size_hint_min_set
       (sd->spacer, 1, (double)sd->size * elm_widget_scale_get(obj) *
       elm_config_scale_get());

   elm_layout_sizing_eval(obj);
}

EAPI Evas_Coord
elm_progressbar_span_size_get(const Evas_Object *obj)
{
   ELM_PROGRESSBAR_CHECK(obj) 0;
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   return sd->size;
}

EAPI void
elm_progressbar_unit_format_set(Evas_Object *obj,
                                const char *units)
{
   ELM_PROGRESSBAR_CHECK(obj);
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   eina_stringshare_replace(&sd->units, units);
   if (units)
     {
        elm_layout_signal_emit(obj, "elm,state,units,visible", "elm");
        edje_object_message_signal_process(ELM_WIDGET_DATA(sd)->resize_obj);
     }
   else
     {
        elm_layout_signal_emit(obj, "elm,state,units,hidden", "elm");
        edje_object_message_signal_process(ELM_WIDGET_DATA(sd)->resize_obj);
     }

   _units_set(obj);
   elm_layout_sizing_eval(obj);
}

EAPI const char *
elm_progressbar_unit_format_get(const Evas_Object *obj)
{
   ELM_PROGRESSBAR_CHECK(obj) NULL;
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   return sd->units;
}

EAPI void
elm_progressbar_horizontal_set(Evas_Object *obj,
                               Eina_Bool horizontal)
{
   ELM_PROGRESSBAR_CHECK(obj);
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   horizontal = !!horizontal;
   if (sd->horizontal == horizontal) return;

   sd->horizontal = horizontal;
   _elm_progressbar_smart_theme(obj);
}

EAPI Eina_Bool
elm_progressbar_horizontal_get(const Evas_Object *obj)
{
   ELM_PROGRESSBAR_CHECK(obj) EINA_FALSE;
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   return sd->horizontal;
}

EAPI void
elm_progressbar_inverted_set(Evas_Object *obj,
                             Eina_Bool inverted)
{
   ELM_PROGRESSBAR_CHECK(obj);
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   inverted = !!inverted;
   if (sd->inverted == inverted) return;

   sd->inverted = inverted;
   if (sd->inverted)
     elm_layout_signal_emit(obj, "elm,state,inverted,on", "elm");
   else
     elm_layout_signal_emit(obj, "elm,state,inverted,off", "elm");

   edje_object_message_signal_process(ELM_WIDGET_DATA(sd)->resize_obj);

   _val_set(obj);
   _units_set(obj);
}

EAPI Eina_Bool
elm_progressbar_inverted_get(const Evas_Object *obj)
{
   ELM_PROGRESSBAR_CHECK(obj) EINA_FALSE;
   ELM_PROGRESSBAR_DATA_GET(obj, sd);

   return sd->inverted;
}
