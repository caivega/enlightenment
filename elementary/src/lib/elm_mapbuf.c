#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_container.h"

static const char MAPBUF_SMART_NAME[] = "elm_mapbuf";

typedef struct _Elm_Mapbuf_Smart_Data Elm_Mapbuf_Smart_Data;

struct _Elm_Mapbuf_Smart_Data
{
   Elm_Widget_Smart_Data base;

   Evas_Object          *content;

   Eina_Bool             enabled : 1;
   Eina_Bool             smooth : 1;
   Eina_Bool             alpha : 1;
};

#define ELM_MAPBUF_DATA_GET(o, sd) \
  Elm_Mapbuf_Smart_Data * sd = evas_object_smart_data_get(o)

#define ELM_MAPBUF_DATA_GET_OR_RETURN(o, ptr)        \
  ELM_MAPBUF_DATA_GET(o, ptr);                       \
  if (!ptr)                                          \
    {                                                \
       CRITICAL("No widget data for object %p (%s)", \
                o, evas_object_type_get(o));         \
       return;                                       \
    }

#define ELM_MAPBUF_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_MAPBUF_DATA_GET(o, ptr);                         \
  if (!ptr)                                            \
    {                                                  \
       CRITICAL("No widget data for object %p (%s)",   \
                o, evas_object_type_get(o));           \
       return val;                                     \
    }

#define ELM_MAPBUF_CHECK(obj)                                             \
  if (!obj || !elm_widget_type_check((obj), MAPBUF_SMART_NAME, __func__)) \
    return

EVAS_SMART_SUBCLASS_NEW
  (MAPBUF_SMART_NAME, _elm_mapbuf, Elm_Container_Smart_Class,
  Elm_Container_Smart_Class, elm_container_smart_class_get, NULL);

static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord maxw = -1, maxh = -1;

   ELM_MAPBUF_DATA_GET(obj, sd);
   if (sd->content)
     {
        evas_object_size_hint_min_get(sd->content, &minw, &minh);
        evas_object_size_hint_max_get(sd->content, &maxw, &maxh);
     }
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static Eina_Bool
_elm_mapbuf_smart_theme(Evas_Object *obj)
{
   if (!ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->theme(obj)) return EINA_FALSE;

   _sizing_eval(obj);

   return EINA_TRUE;
}

static void
_changed_size_hints_cb(void *data,
                       Evas *e __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static Eina_Bool
_elm_mapbuf_smart_sub_object_del(Evas_Object *obj,
                                 Evas_Object *sobj)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->sub_object_del(obj, sobj))
     return EINA_FALSE;

   if (sobj == sd->content)
     {
        evas_object_data_del(sobj, "_elm_leaveme");
        evas_object_smart_member_del(sobj);
        evas_object_clip_unset(sobj);
        evas_object_event_callback_del_full
          (sobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb,
          obj);
        sd->content = NULL;
        _sizing_eval(obj);
     }

   return EINA_TRUE;
}

static void
_mapbuf(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   ELM_MAPBUF_DATA_GET(obj, sd);

   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, &x, &y, &w, &h);
   evas_object_resize(sd->content, w, h);

   if (sd->enabled)
     {
        Evas_Map *m;

        m = evas_map_new(4);
        evas_map_util_points_populate_from_geometry(m, x, y, w, h, 0);
        evas_map_smooth_set(m, sd->smooth);
        evas_map_alpha_set(m, sd->alpha);
        evas_object_map_set(sd->content, m);
        evas_object_map_enable_set(sd->content, EINA_TRUE);
        evas_map_free(m);
     }
   else
     {
        evas_object_map_set(sd->content, NULL);
        evas_object_map_enable_set(sd->content, EINA_FALSE);
        evas_object_move(sd->content, x, y);
     }
}

static void
_configure(Evas_Object *obj)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->content)
     {
        Evas_Coord x, y, w, h, x2, y2;

        evas_object_geometry_get
          (ELM_WIDGET_DATA(sd)->resize_obj, &x, &y, &w, &h);
        evas_object_geometry_get(sd->content, &x2, &y2, NULL, NULL);
        if ((x != x2) || (y != y2))
          {
             if (!sd->enabled)
               evas_object_move(sd->content, x, y);
             else
               {
                  Evas *e = evas_object_evas_get(obj);
                  evas_smart_objects_calculate(e);
                  evas_nochange_push(e);
                  evas_object_move(sd->content, x, y);
                  evas_smart_objects_calculate(e);
                  evas_nochange_pop(e);
               }
          }
        _mapbuf(obj);
     }
}

static void
_elm_mapbuf_smart_move(Evas_Object *obj,
                       Evas_Coord x,
                       Evas_Coord y)
{
   ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->base.move(obj, x, y);

   _configure(obj);
}

static void
_elm_mapbuf_smart_resize(Evas_Object *obj,
                         Evas_Coord x,
                         Evas_Coord y)
{
   ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->base.resize(obj, x, y);

   _configure(obj);
}

static Eina_Bool
_elm_mapbuf_smart_content_set(Evas_Object *obj,
                              const char *part,
                              Evas_Object *content)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return EINA_FALSE;
   if (sd->content == content) return EINA_TRUE;

   if (sd->content) evas_object_del(sd->content);
   sd->content = content;

   if (content)
     {
        evas_object_data_set(content, "_elm_leaveme", (void *)1);
        elm_widget_sub_object_add(obj, content);
        evas_object_smart_member_add(content, obj);
        evas_object_clip_set(content, ELM_WIDGET_DATA(sd)->resize_obj);
        evas_object_color_set
          (ELM_WIDGET_DATA(sd)->resize_obj, 255, 255, 255, 255);
        evas_object_event_callback_add
          (content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
     }
   else
     evas_object_color_set(ELM_WIDGET_DATA(sd)->resize_obj, 0, 0, 0, 0);

   _sizing_eval(obj);
   _configure(obj);

   return EINA_TRUE;
}

static Evas_Object *
_elm_mapbuf_smart_content_get(const Evas_Object *obj,
                              const char *part)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return NULL;
   return sd->content;
}

static Evas_Object *
_elm_mapbuf_smart_content_unset(Evas_Object *obj,
                                const char *part)
{
   Evas_Object *content;

   ELM_MAPBUF_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return NULL;
   if (!sd->content) return NULL;

   content = sd->content;
   elm_widget_sub_object_del(obj, content);
   evas_object_smart_member_del(content);
   evas_object_data_del(content, "_elm_leaveme");
   evas_object_color_set(ELM_WIDGET_DATA(sd)->resize_obj, 0, 0, 0, 0);
   return content;
}

static void
_elm_mapbuf_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Mapbuf_Smart_Data);

   ELM_WIDGET_DATA(priv)->resize_obj =
       evas_object_rectangle_add(evas_object_evas_get(obj));

   ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->base.add(obj);

   evas_object_static_clip_set(ELM_WIDGET_DATA(priv)->resize_obj, EINA_TRUE);
   evas_object_pass_events_set(ELM_WIDGET_DATA(priv)->resize_obj, EINA_TRUE);
   evas_object_color_set(ELM_WIDGET_DATA(priv)->resize_obj, 0, 0, 0, 0);

   priv->enabled = 0;
   priv->alpha = 1;
   priv->smooth = 1;

   elm_widget_can_focus_set(obj, EINA_FALSE);

   _sizing_eval(obj);
}

static void
_elm_mapbuf_smart_set_user(Elm_Container_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_mapbuf_smart_add;
   ELM_WIDGET_CLASS(sc)->base.resize = _elm_mapbuf_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.move = _elm_mapbuf_smart_move;

   ELM_WIDGET_CLASS(sc)->theme = _elm_mapbuf_smart_theme;
   ELM_WIDGET_CLASS(sc)->sub_object_del = _elm_mapbuf_smart_sub_object_del;

   sc->content_set = _elm_mapbuf_smart_content_set;
   sc->content_get = _elm_mapbuf_smart_content_get;
   sc->content_unset = _elm_mapbuf_smart_content_unset;
}

EAPI Evas_Object *
elm_mapbuf_add(Evas_Object *parent)
{
   Evas *e;
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   e = evas_object_evas_get(parent);
   if (!e) return NULL;

   obj = evas_object_smart_add(e, _elm_mapbuf_smart_class_new());

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   return obj;
}

EAPI void
elm_mapbuf_enabled_set(Evas_Object *obj,
                       Eina_Bool enabled)
{
   ELM_MAPBUF_CHECK(obj);
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->enabled == enabled) return;
   sd->enabled = enabled;

   if (sd->content) evas_object_static_clip_set(sd->content, sd->enabled);
   _configure(obj);
}

EAPI Eina_Bool
elm_mapbuf_enabled_get(const Evas_Object *obj)
{
   ELM_MAPBUF_CHECK(obj) EINA_FALSE;
   ELM_MAPBUF_DATA_GET(obj, sd);

   return sd->enabled;
}

EAPI void
elm_mapbuf_smooth_set(Evas_Object *obj,
                      Eina_Bool smooth)
{
   ELM_MAPBUF_CHECK(obj);
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->smooth == smooth) return;
   sd->smooth = smooth;
   _configure(obj);
}

EAPI Eina_Bool
elm_mapbuf_smooth_get(const Evas_Object *obj)
{
   ELM_MAPBUF_CHECK(obj) EINA_FALSE;
   ELM_MAPBUF_DATA_GET(obj, sd);

   return sd->smooth;
}

EAPI void
elm_mapbuf_alpha_set(Evas_Object *obj,
                     Eina_Bool alpha)
{
   ELM_MAPBUF_CHECK(obj);
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->alpha == alpha) return;
   sd->alpha = alpha;
   _configure(obj);
}

EAPI Eina_Bool
elm_mapbuf_alpha_get(const Evas_Object *obj)
{
   ELM_MAPBUF_CHECK(obj) EINA_FALSE;
   ELM_MAPBUF_DATA_GET(obj, sd);

   return sd->alpha;
}
