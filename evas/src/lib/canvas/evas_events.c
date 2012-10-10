#include "evas_common.h"
#include "evas_private.h"

static Eina_List *
_evas_event_object_list_in_get(Evas *eo_e, Eina_List *in,
                               const Eina_Inlist *list, Evas_Object *stop,
                               int x, int y, int *no_rep);

static void
_evas_event_havemap_adjust(Evas_Object *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj, Evas_Coord *x, Evas_Coord *y, Eina_Bool mouse_grabbed)
{
   if (obj->smart.parent)
     {
        Evas_Object_Protected_Data *smart_parent_obj = eo_data_get(obj->smart.parent, EVAS_OBJ_CLASS);
        _evas_event_havemap_adjust(obj->smart.parent, smart_parent_obj, x, y, mouse_grabbed);
     }

   if ((!obj->cur.usemap) || (!obj->cur.map) || (!obj->cur.map->count == 4))
      return;

   evas_map_coords_get(obj->cur.map, *x, *y, x, y, mouse_grabbed);
   *x += obj->cur.geometry.x;
   *y += obj->cur.geometry.y;
}

static void
_evas_event_framespace_adjust(Evas_Object *eo_obj, Evas_Coord *x, Evas_Coord *y)
{
  Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
  if (obj->is_frame) return;
  
  if ((!obj->smart.parent) && (obj->is_smart))
    {
       Evas_Public_Data *evas;

       evas = obj->layer->evas;
       if (x) *x -= evas->framespace.x;
       if (y) *y -= evas->framespace.y;
    }
}

static Eina_List *
_evas_event_object_list_raw_in_get(Evas *eo_e, Eina_List *in,
                                   const Eina_Inlist *list, Evas_Object *stop,
                                   int x, int y, int *no_rep)
{
   Evas_Object *eo_obj;
   Evas_Object_Protected_Data *obj;
   int inside;

   if (!list) return in;
   for (obj = _EINA_INLIST_CONTAINER(obj, list); 
        obj; 
        obj = _EINA_INLIST_CONTAINER(obj, EINA_INLIST_GET(obj)->prev))
     {
        eo_obj = obj->object;
        if (eo_obj == stop)
          {
             *no_rep = 1;
             return in;
          }
        if (evas_event_passes_through(eo_obj, obj)) continue;
        if ((obj->cur.visible) && (obj->delete_me == 0) &&
            (!obj->clip.clipees) &&
            (evas_object_clippers_is_visible(eo_obj, obj)))
          {
             if (obj->is_smart)
               {
                  int norep = 0;

                  if ((obj->cur.usemap) && (obj->cur.map) &&
                      (obj->cur.map->count == 4))
                    {
                       inside = evas_object_is_in_output_rect(eo_obj, obj, x, y, 1, 1);
                       if (inside)
                         {
                            if (!evas_map_coords_get(obj->cur.map, x, y,
                                                     &(obj->cur.map->mx),
                                                     &(obj->cur.map->my), 0))
                              {
                                 inside = 0;
                              }
                            else
                              {
                                 in = _evas_event_object_list_in_get
                                    (eo_e, in,
                                     evas_object_smart_members_get_direct(eo_obj),
                                     stop,
                                     obj->cur.geometry.x + obj->cur.map->mx,
                                     obj->cur.geometry.y + obj->cur.map->my,
                                     &norep);
                              }
                         }
                    }
                  else
                    {
                       if (!obj->child_has_map)
                         evas_object_smart_bounding_box_update(eo_obj, obj);
                       if (obj->child_has_map ||
                           (obj->cur.bounding_box.x <= x &&
                            obj->cur.bounding_box.x + obj->cur.bounding_box.w >= x &&
                            obj->cur.bounding_box.y <= y &&
                            obj->cur.bounding_box.y + obj->cur.bounding_box.h >= y) ||
                           (obj->cur.geometry.x <= x &&
                            obj->cur.geometry.y + obj->cur.geometry.w >= x &&
                            obj->cur.geometry.y <= y &&
                            obj->cur.geometry.y + obj->cur.geometry.h >= y))
                         in = _evas_event_object_list_in_get
                            (eo_e, in, evas_object_smart_members_get_direct(eo_obj),
                            stop, x, y, &norep);
                    }
                  if (norep)
                    {
                       if (!obj->repeat_events)
                         {
                            *no_rep = 1;
                            return in;
                         }
                    }
               }
             else
               {
                  inside = evas_object_is_in_output_rect(eo_obj, obj, x, y, 1, 1);

                  if (inside)
                    {
                       if ((obj->cur.usemap) && (obj->cur.map) &&
                           (obj->cur.map->count == 4))
                         {
                            if (!evas_map_coords_get(obj->cur.map, x, y,
                                                     &(obj->cur.map->mx),
                                                     &(obj->cur.map->my), 0))
                              {
                                 inside = 0;
                              }
                         }
                    }
                  if (inside && ((!obj->precise_is_inside) ||
                                 (evas_object_is_inside(eo_obj, obj, x, y))))
                    {
                       if (!evas_event_freezes_through(eo_obj, obj))
                         in = eina_list_append(in, eo_obj);
                       if (!obj->repeat_events)
                         {
                            *no_rep = 1;
                            return in;
                         }
                    }
               }
          }
     }
   *no_rep = 0;
   return in;
}

static Eina_List *
_evas_event_object_list_in_get(Evas *eo_e, Eina_List *in,
                               const Eina_Inlist *list, Evas_Object *stop,
                               int x, int y, int *no_rep)
{
   if (!list) return NULL;
   return _evas_event_object_list_raw_in_get(eo_e, in, list->last, stop, x, y,
                                             no_rep);
}

Eina_List *
evas_event_objects_event_list(Evas *eo_e, Evas_Object *stop, int x, int y)
{
   Evas_Public_Data *e = eo_data_get(eo_e, EVAS_CLASS);
   Evas_Layer *lay;
   Eina_List *in = NULL;

   if ((!e->layers) || (e->is_frozen)) return NULL;
   EINA_INLIST_REVERSE_FOREACH((EINA_INLIST_GET(e->layers)), lay)
     {
        int no_rep = 0;
        in = _evas_event_object_list_in_get(eo_e, in,
                                            EINA_INLIST_GET(lay->objects),
                                            stop, x, y, &no_rep);
        if (no_rep) return in;
     }
   return in;
}

static Eina_List *
evas_event_list_copy(Eina_List *list)
{
   Eina_List *l, *new_l = NULL;
   const void *data;

   EINA_LIST_FOREACH(list, l, data)
     new_l = eina_list_append(new_l, data);
   return new_l;
}
/* public functions */

EAPI void
evas_event_default_flags_set(Evas *eo_e, Evas_Event_Flags flags)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_e, evas_canvas_event_default_flags_set(flags));
}

void
_canvas_event_default_flags_set(Eo *eo_e EINA_UNUSED, void *_pd, va_list *list)
{
   Evas_Event_Flags flags = va_arg(*list, Evas_Event_Flags);
   Evas_Public_Data *e = _pd;
   e->default_event_flags = flags;
}

EAPI Evas_Event_Flags
evas_event_default_flags_get(const Evas *eo_e)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return EVAS_EVENT_FLAG_ON_HOLD;
   MAGIC_CHECK_END();
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_ON_HOLD;
   eo_do((Eo *)eo_e, evas_canvas_event_default_flags_get(&flags));
   return flags;
}

void
_canvas_event_default_flags_get(Eo *eo_e EINA_UNUSED, void *_pd, va_list *list)
{
   Evas_Event_Flags *ret = va_arg(*list, Evas_Event_Flags *);
   const Evas_Public_Data *e = _pd;
   *ret = e->default_event_flags;
}

static inline void
_canvas_event_thaw_eval_internal(Eo *eo_e, Evas_Public_Data *e)
{
   evas_event_feed_mouse_move(eo_e, e->pointer.x, e->pointer.y,
         e->last_timestamp, NULL);
}

EAPI void
evas_event_freeze(Evas *eo_e)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_e, eo_event_freeze());
}

EAPI void
evas_event_thaw(Evas *eo_e)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_e, eo_event_thaw());
}

void
_canvas_event_freeze(Eo *eo_e, void *_pd, va_list *list EINA_UNUSED)
{
   eo_do_super(eo_e, eo_event_freeze());
   Evas_Public_Data *e = _pd;
   e->is_frozen = EINA_TRUE;
}

void
_canvas_event_thaw(Eo *eo_e, void *_pd, va_list *list EINA_UNUSED)
{
   int fcount = -1;
   eo_do_super(eo_e,
         eo_event_thaw(),
         eo_event_freeze_get(&fcount));
   if (0 == fcount)
     {
        Evas_Public_Data *e = _pd;
        Evas_Layer *lay;

        e->is_frozen = EINA_FALSE;
        EINA_INLIST_FOREACH((EINA_INLIST_GET(e->layers)), lay)
          {
             Evas_Object_Protected_Data *obj;

             EINA_INLIST_FOREACH(lay->objects, obj)
               {
                  Evas_Object *eo_obj = obj->object;
                  evas_object_clip_recalc(eo_obj, obj);
                  evas_object_recalc_clippees(eo_obj, obj);
               }
          }

        _canvas_event_thaw_eval_internal(eo_e, e);
     }
}

EAPI int
evas_event_freeze_get(const Evas *eo_e)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return 0;
   MAGIC_CHECK_END();
   int ret = 0;
   eo_do((Eo *)eo_e, eo_event_freeze_get(&ret));
   return ret;
}

EAPI void
evas_event_thaw_eval(Evas *eo_e)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();
   Evas_Public_Data *e = eo_data_get(eo_e, EVAS_CLASS);
   if (0 == evas_event_freeze_get(eo_e))
     {
        _canvas_event_thaw_eval_internal(eo_e, e);
     }
}

EAPI void
evas_event_feed_mouse_down(Evas *eo_e, int b, Evas_Button_Flags flags, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_e, evas_canvas_event_feed_mouse_down(b, flags, timestamp, data));
}

void
_canvas_event_feed_mouse_down(Eo *eo_e, void *_pd, va_list *list)
{
   int b = va_arg(*list, int);
   Evas_Button_Flags flags = va_arg(*list, Evas_Button_Flags);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Eina_List *l, *copy;
   Evas_Event_Mouse_Down ev;
   Evas_Object *eo_obj;
   int addgrab = 0;
   int event_id = 0;

   if ((b < 1) || (b > 32)) return;

   e->pointer.button |= (1 << (b - 1));
   e->pointer.downs++;

   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   ev.button = b;
   ev.output.x = e->pointer.x;
   ev.output.y = e->pointer.y;
   ev.canvas.x = e->pointer.x;
   ev.canvas.y = e->pointer.y;
   ev.data = (void *)data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.flags = flags;
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);

   _evas_walk(e);
   /* append new touch point to the touch point list */
   _evas_touch_point_append(eo_e, 0, e->pointer.x, e->pointer.y);
   /* If this is the first finger down, i.e no other fingers pressed,
    * get a new event list, otherwise, keep the current grabbed list. */
   if (e->pointer.mouse_grabbed == 0)
     {
        Eina_List *ins = evas_event_objects_event_list(eo_e,
                                                       NULL,
                                                       e->pointer.x,
                                                       e->pointer.y);
        /* free our old list of ins */
        e->pointer.object.in = eina_list_free(e->pointer.object.in);
        /* and set up the new one */
        e->pointer.object.in = ins;
        /* adjust grabbed count by the nuymber of currently held down
         * fingers/buttons */
        if (e->pointer.downs > 1) addgrab = e->pointer.downs - 1;
     }
   copy = evas_event_list_copy(e->pointer.object.in);
   EINA_LIST_FOREACH(copy, l, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        if ((obj->pointer_mode == EVAS_OBJECT_POINTER_MODE_AUTOGRAB) ||
            (obj->pointer_mode == EVAS_OBJECT_POINTER_MODE_NOGRAB_NO_REPEAT_UPDOWN))
          {
             obj->mouse_grabbed += addgrab + 1;
             e->pointer.mouse_grabbed += addgrab + 1;
             if (obj->pointer_mode == EVAS_OBJECT_POINTER_MODE_NOGRAB_NO_REPEAT_UPDOWN)
               {
                  e->pointer.nogrep++;
                  break;
               }
          }
     }
   EINA_LIST_FOREACH(copy, l, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        if (obj->delete_me) continue;
        ev.canvas.x = e->pointer.x;
        ev.canvas.y = e->pointer.y;
        _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
        _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);

        if (!e->is_frozen)
          evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_DOWN, &ev, event_id);
        if (e->delete_me) break;
        if (obj->pointer_mode == EVAS_OBJECT_POINTER_MODE_NOGRAB_NO_REPEAT_UPDOWN)
          break;
     }
   if (copy) eina_list_free(copy);
   e->last_mouse_down_counter++;
   _evas_post_event_callback_call(eo_e, e);
   /* update touch point's state to EVAS_TOUCH_POINT_STILL */
   _evas_touch_point_update(eo_e, 0, e->pointer.x, e->pointer.y, EVAS_TOUCH_POINT_STILL);
   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
}

static int
_post_up_handle(Evas *eo_e, unsigned int timestamp, const void *data)
{
   Evas_Public_Data *e = eo_data_get(eo_e, EVAS_CLASS);
   Eina_List *l, *copy, *ins, *ll;
   Evas_Event_Mouse_Out ev;
   Evas_Object *eo_obj;
   int post_called = 0;
   int event_id = 0;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   ev.buttons = e->pointer.button;
   ev.output.x = e->pointer.x;
   ev.output.y = e->pointer.y;
   ev.canvas.x = e->pointer.x;
   ev.canvas.y = e->pointer.y;
   ev.data = (void *)data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   /* get new list of ins */
   ins = evas_event_objects_event_list(eo_e, NULL, e->pointer.x, e->pointer.y);
   /* go thru old list of in objects */
   copy = evas_event_list_copy(e->pointer.object.in);
   EINA_LIST_FOREACH(copy, ll, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        ev.canvas.x = e->pointer.x;
        ev.canvas.y = e->pointer.y;
        _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
        _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);
        if ((!eina_list_data_find(ins, eo_obj)) ||
            (!e->pointer.inside))
          {
             if (obj->mouse_in)
               {
                  obj->mouse_in = 0;
                  if (!e->is_frozen)
                     evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_OUT, &ev, event_id);
               }
          }
        if (e->delete_me) break;
     }
   _evas_post_event_callback_call(eo_e, e);

   if (copy) copy = eina_list_free(copy);
   if (e->pointer.inside)
     {
        Evas_Event_Mouse_In ev_in;
        Evas_Object *eo_obj_itr;

        _evas_object_event_new();

        event_id = _evas_event_counter;
        ev_in.buttons = e->pointer.button;
        ev_in.output.x = e->pointer.x;
        ev_in.output.y = e->pointer.y;
        ev_in.canvas.x = e->pointer.x;
        ev_in.canvas.y = e->pointer.y;
        ev_in.data = (void *)data;
        ev_in.modifiers = &(e->modifiers);
        ev_in.locks = &(e->locks);
        ev_in.timestamp = timestamp;
        ev_in.event_flags = e->default_event_flags;

        EINA_LIST_FOREACH(ins, l, eo_obj_itr)
          {
             Evas_Object_Protected_Data *obj_itr = eo_data_get(eo_obj_itr, EVAS_OBJ_CLASS);
             ev_in.canvas.x = e->pointer.x;
             ev_in.canvas.y = e->pointer.y;
             _evas_event_framespace_adjust(eo_obj_itr, &ev_in.canvas.x, &ev_in.canvas.y);
             _evas_event_havemap_adjust(eo_obj_itr, obj_itr, &ev_in.canvas.x, &ev_in.canvas.y, obj_itr->mouse_grabbed);
             if (!eina_list_data_find(e->pointer.object.in, eo_obj_itr))
               {
                  if (!obj_itr->mouse_in)
                    {
                       obj_itr->mouse_in = 1;
                       if (!e->is_frozen)
                          evas_object_event_callback_call(eo_obj_itr, obj_itr, EVAS_CALLBACK_MOUSE_IN, &ev_in, event_id);
                    }
               }
             if (e->delete_me) break;
          }
        post_called = 1;
        _evas_post_event_callback_call(eo_e, e);
     }
   else
     {
        ins = eina_list_free(ins);
     }

   if (e->pointer.mouse_grabbed == 0)
     {
        /* free our old list of ins */
        eina_list_free(e->pointer.object.in);
        /* and set up the new one */
        e->pointer.object.in = ins;
     }
   else
     {
        /* free our cur ins */
        eina_list_free(ins);
     }
   if (e->pointer.inside)
      evas_event_feed_mouse_move(eo_e, e->pointer.x, e->pointer.y, timestamp, data);
   if (ev.dev) _evas_device_unref(ev.dev);
   return post_called;
}

EAPI void
evas_event_feed_mouse_up(Evas *eo_e, int b, Evas_Button_Flags flags, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_mouse_up(b, flags, timestamp, data));
}

void
_canvas_event_feed_mouse_up(Eo *eo_e, void *_pd, va_list *list)
{
   int b = va_arg(*list, int);
   Evas_Button_Flags flags = va_arg(*list, Evas_Button_Flags);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Eina_List *l, *copy;

   if ((b < 1) || (b > 32)) return;
   if (e->pointer.downs <= 0) return;

   e->pointer.button &= ~(1 << (b - 1));
   e->pointer.downs--;

   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

     {
        Evas_Event_Mouse_Up ev;
        Evas_Object *eo_obj;
        int event_id = 0;

        _evas_object_event_new();

        event_id = _evas_event_counter;
        ev.button = b;
        ev.output.x = e->pointer.x;
        ev.output.y = e->pointer.y;
        ev.canvas.x = e->pointer.x;
        ev.canvas.y = e->pointer.y;
        ev.data = (void *)data;
        ev.modifiers = &(e->modifiers);
        ev.locks = &(e->locks);
        ev.flags = flags;
        ev.timestamp = timestamp;
        ev.event_flags = e->default_event_flags;
        ev.dev = _evas_device_top_get(eo_e);
        if (ev.dev) _evas_device_ref(ev.dev);
        
        _evas_walk(e);
        /* update released touch point */
        _evas_touch_point_update(eo_e, 0, e->pointer.x, e->pointer.y, EVAS_TOUCH_POINT_UP);
        copy = evas_event_list_copy(e->pointer.object.in);
        EINA_LIST_FOREACH(copy, l, eo_obj)
          {
             Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
             ev.canvas.x = e->pointer.x;
             ev.canvas.y = e->pointer.y;
             _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
             _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);
             if ((obj->pointer_mode == EVAS_OBJECT_POINTER_MODE_AUTOGRAB) &&
                 (obj->mouse_grabbed > 0))
               {
                  obj->mouse_grabbed--;
                  e->pointer.mouse_grabbed--;
               }
             if (!obj->delete_me)
               {
                  if ((!e->is_frozen) &&
                      (!evas_event_freezes_through(eo_obj, obj)))
                      evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_UP, &ev, event_id);
               }
             if (e->delete_me) break;
             if (obj->pointer_mode == EVAS_OBJECT_POINTER_MODE_NOGRAB_NO_REPEAT_UPDOWN)
               {
                  if (e->pointer.nogrep > 0) e->pointer.nogrep--;
                  break;
               }
          }
        if (copy) copy = eina_list_free(copy);
        e->last_mouse_up_counter++;
        _evas_post_event_callback_call(eo_e, e);
        if (ev.dev) _evas_device_unref(ev.dev);
     }

   if (e->pointer.mouse_grabbed == 0)
     {
        _post_up_handle(eo_e, timestamp, data);
     }

   if (e->pointer.mouse_grabbed < 0)
     {
        ERR("BUG? e->pointer.mouse_grabbed (=%d) < 0!",
            e->pointer.mouse_grabbed);
     }
   /* remove released touch point from the touch point list */
   _evas_touch_point_remove(eo_e, 0);

   _evas_unwalk(e);
}

EAPI void
evas_event_feed_mouse_cancel(Evas *eo_e, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_mouse_cancel(timestamp, data));
}

void
_canvas_event_feed_mouse_cancel(Eo *eo_e, void *_pd, va_list *list)
{
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   int i;

   if (e->is_frozen) return;

   _evas_walk(e);
   for (i = 0; i < 32; i++)
     {
        if ((e->pointer.button & (1 << i)))
          evas_event_feed_mouse_up(eo_e, i + 1, 0, timestamp, data);
     }
   // FIXME: multi cancel too?
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_mouse_wheel(Evas *eo_e, int direction, int z, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_mouse_wheel(direction, z, timestamp, data));
}

void
_canvas_event_feed_mouse_wheel(Eo *eo_e, void *_pd, va_list *list)
{
   int direction = va_arg(*list, int);
   int z = va_arg(*list, int);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Eina_List *l, *copy;
   Evas_Event_Mouse_Wheel ev;
   Evas_Object *eo_obj;
   int event_id = 0;

   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   ev.direction = direction;
   ev.z = z;
   ev.output.x = e->pointer.x;
   ev.output.y = e->pointer.y;
   ev.canvas.x = e->pointer.x;
   ev.canvas.y = e->pointer.y;
   ev.data = (void *) data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   _evas_walk(e);
   copy = evas_event_list_copy(e->pointer.object.in);

   EINA_LIST_FOREACH(copy, l, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        ev.canvas.x = e->pointer.x;
        ev.canvas.y = e->pointer.y;
        _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
        _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);
        if ((!e->is_frozen) && !evas_event_freezes_through(eo_obj, obj))
          evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_WHEEL, &ev, event_id);
        if (e->delete_me) break;
     }
   if (copy) copy = eina_list_free(copy);
   _evas_post_event_callback_call(eo_e, e);

   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_mouse_move(Evas *eo_e, int x, int y, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_mouse_move(x, y, timestamp, data));
}

void
_canvas_event_feed_mouse_move(Eo *eo_e, void *_pd, va_list *list)
{
   int x = va_arg(*list, int);
   int y = va_arg(*list, int);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Evas_Object *nogrep_obj = NULL;
   int px, py;
////   Evas_Coord pcx, pcy;

   px = e->pointer.x;
   py = e->pointer.y;
////   pcx = e->pointer.canvas_x;
////   pcy = e->pointer.canvas_y;

   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   e->pointer.x = x;
   e->pointer.y = y;
////   e->pointer.canvas_x = x;
////   e->pointer.canvas_y = y;
////   e->pointer.canvas_x = evas_coord_screen_x_to_world(eo_e, x);
////   e->pointer.canvas_y = evas_coord_screen_y_to_world(eo_e, y);
   if ((!e->pointer.inside) && (e->pointer.mouse_grabbed == 0)) return;
   _evas_walk(e);
   /* update moved touch point */
   if ((px != x) || (py != y))
     _evas_touch_point_update(eo_e, 0, e->pointer.x, e->pointer.y, EVAS_TOUCH_POINT_MOVE);
   /* if our mouse button is grabbed to any objects */
   if (e->pointer.mouse_grabbed > 0)
     {
        /* go thru old list of in objects */
        Eina_List *outs = NULL;
        Eina_List *l, *copy;

          {
             Evas_Event_Mouse_Move ev;
             Evas_Object *eo_obj;
             int event_id = 0;

             _evas_object_event_new();

             event_id = _evas_event_counter;
             ev.buttons = e->pointer.button;
             ev.cur.output.x = e->pointer.x;
             ev.cur.output.y = e->pointer.y;
             ev.cur.canvas.x = e->pointer.x;
             ev.cur.canvas.y = e->pointer.y;
             ev.prev.output.x = px;
             ev.prev.output.y = py;
             ev.prev.canvas.x = px;
             ev.prev.canvas.y = py;
             ev.data = (void *)data;
             ev.modifiers = &(e->modifiers);
             ev.locks = &(e->locks);
             ev.timestamp = timestamp;
             ev.event_flags = e->default_event_flags;
             ev.dev = _evas_device_top_get(eo_e);
             if (ev.dev) _evas_device_ref(ev.dev);
             copy = evas_event_list_copy(e->pointer.object.in);
             EINA_LIST_FOREACH(copy, l, eo_obj)
               {
                  Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
                  ev.cur.canvas.x = e->pointer.x;
                  ev.cur.canvas.y = e->pointer.y;
                  _evas_event_framespace_adjust(eo_obj, &ev.cur.canvas.x, &ev.cur.canvas.y);
                  _evas_event_havemap_adjust(eo_obj, obj, &ev.cur.canvas.x,
                                             &ev.cur.canvas.y,
                                             obj->mouse_grabbed);
                  if ((!e->is_frozen) &&
                      (evas_object_clippers_is_visible(eo_obj, obj) ||
                       obj->mouse_grabbed) &&
                      (!evas_event_passes_through(eo_obj, obj)) &&
                      (!evas_event_freezes_through(eo_obj, obj)) &&
                      (!obj->clip.clipees))
                    {
                       if ((px != x) || (py != y))
                         evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_MOVE, &ev, event_id);
                    }
                  else
                    outs = eina_list_append(outs, eo_obj);
                  if ((obj->pointer_mode == EVAS_OBJECT_POINTER_MODE_NOGRAB_NO_REPEAT_UPDOWN) &&
                      (e->pointer.nogrep > 0))
                    {
                       eina_list_free(copy);
                       nogrep_obj = eo_obj;
                       goto nogrep;
                    }
                  if (e->delete_me) break;
               }
             _evas_post_event_callback_call(eo_e, e);
             if (ev.dev) _evas_device_unref(ev.dev);
          }
          {
             Evas_Event_Mouse_Out ev;
             int event_id = 0;

             _evas_object_event_new();

             event_id = _evas_event_counter;
             ev.buttons = e->pointer.button;
             ev.output.x = e->pointer.x;
             ev.output.y = e->pointer.y;
             ev.canvas.x = e->pointer.x;
             ev.canvas.y = e->pointer.y;
             ev.data = (void *)data;
             ev.modifiers = &(e->modifiers);
             ev.locks = &(e->locks);
             ev.timestamp = timestamp;
             ev.event_flags = e->default_event_flags;
             ev.dev = _evas_device_top_get(eo_e);
             if (ev.dev) _evas_device_ref(ev.dev);
             
             if (copy) eina_list_free(copy);
             while (outs)
               {
                  Evas_Object *eo_obj;

                  eo_obj = outs->data;
                  outs = eina_list_remove(outs, eo_obj);
                  Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
                  if ((obj->mouse_grabbed == 0) && (!e->delete_me))
                    {
                       ev.canvas.x = e->pointer.x;
                       ev.canvas.y = e->pointer.y;
                       _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
                       _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);
                       e->pointer.object.in = eina_list_remove(e->pointer.object.in, eo_obj);
                       if (obj->mouse_in)
                         {
                            obj->mouse_in = 0;
                            if (!obj->delete_me)
                              {
                                 if (!e->is_frozen)
                                    evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_OUT, &ev, event_id);
                              }
                         }
                    }
               }
             _evas_post_event_callback_call(eo_e, e);
             if (ev.dev) _evas_device_unref(ev.dev);
          }
     }
   else
     {
        Eina_List *ins;
        Eina_List *l, *copy;
        Evas_Event_Mouse_Move ev;
        Evas_Event_Mouse_Out ev2;
        Evas_Event_Mouse_In ev3;
        Evas_Object *eo_obj;
        int event_id = 0, event_id2 = 0;

        _evas_object_event_new();

        event_id = _evas_event_counter;
        ev.buttons = e->pointer.button;
        ev.cur.output.x = e->pointer.x;
        ev.cur.output.y = e->pointer.y;
        ev.cur.canvas.x = e->pointer.x;
        ev.cur.canvas.y = e->pointer.y;
        ev.prev.output.x = px;
        ev.prev.output.y = py;
        ev.prev.canvas.x = px;
        ev.prev.canvas.y = py;
        ev.data = (void *)data;
        ev.modifiers = &(e->modifiers);
        ev.locks = &(e->locks);
        ev.timestamp = timestamp;
        ev.event_flags = e->default_event_flags;
        ev.dev = _evas_device_top_get(eo_e);
        if (ev.dev) _evas_device_ref(ev.dev);
        
        ev2.buttons = e->pointer.button;
        ev2.output.x = e->pointer.x;
        ev2.output.y = e->pointer.y;
        ev2.canvas.x = e->pointer.x;
        ev2.canvas.y = e->pointer.y;
        ev2.data = (void *)data;
        ev2.modifiers = &(e->modifiers);
        ev2.locks = &(e->locks);
        ev2.timestamp = timestamp;
        ev2.event_flags = e->default_event_flags;
        ev2.dev = ev.dev;

        ev3.buttons = e->pointer.button;
        ev3.output.x = e->pointer.x;
        ev3.output.y = e->pointer.y;
        ev3.canvas.x = e->pointer.x;
        ev3.canvas.y = e->pointer.y;
        ev3.data = (void *)data;
        ev3.modifiers = &(e->modifiers);
        ev3.locks = &(e->locks);
        ev3.timestamp = timestamp;
        ev3.event_flags = e->default_event_flags;
        ev3.dev = ev.dev;

        /* get all new in objects */
        ins = evas_event_objects_event_list(eo_e, NULL, x, y);
        /* go thru old list of in objects */
        copy = evas_event_list_copy(e->pointer.object.in);
        EINA_LIST_FOREACH(copy, l, eo_obj)
          {
             Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
             /* if its under the pointer and its visible and its in the new */
             /* in list */
             // FIXME: i don't think we need this
             //	     evas_object_clip_recalc(eo_obj);
             if ((!e->is_frozen) &&
                 evas_object_is_in_output_rect(eo_obj, obj, x, y, 1, 1) &&
                 (evas_object_clippers_is_visible(eo_obj, obj) ||
                  obj->mouse_grabbed) &&
                 eina_list_data_find(ins, eo_obj) &&
                 (!evas_event_passes_through(eo_obj, obj)) &&
                 (!evas_event_freezes_through(eo_obj, obj)) &&
                 (!obj->clip.clipees) &&
                 ((!obj->precise_is_inside) || evas_object_is_inside(eo_obj, obj, x, y))
                )
               {
                  if ((px != x) || (py != y))
                    {
                       ev.cur.canvas.x = e->pointer.x;
                       ev.cur.canvas.y = e->pointer.y;
                       _evas_event_havemap_adjust(eo_obj, obj, &ev.cur.canvas.x, &ev.cur.canvas.y, obj->mouse_grabbed);
                       evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_MOVE, &ev, event_id);
                    }
               }
             /* otherwise it has left the object */
             else
               {
                  if (obj->mouse_in)
                    {
                       obj->mouse_in = 0;
                       ev2.canvas.x = e->pointer.x;
                       ev2.canvas.y = e->pointer.y;
                       _evas_event_framespace_adjust(eo_obj, &ev2.canvas.x, &ev2.canvas.y);
                       _evas_event_havemap_adjust(eo_obj, obj, &ev2.canvas.x, &ev2.canvas.y, obj->mouse_grabbed);
                       if (!e->is_frozen)
                          evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_OUT, &ev2, event_id);
                    }
               }
             if (e->delete_me) break;
          }
        _evas_post_event_callback_call(eo_e, e);

        _evas_object_event_new();

        event_id2 = _evas_event_counter;
        if (copy) copy = eina_list_free(copy);
        /* go thru our current list of ins */
        EINA_LIST_FOREACH(ins, l, eo_obj)
          {
             Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
             ev3.canvas.x = e->pointer.x;
             ev3.canvas.y = e->pointer.y;
             _evas_event_framespace_adjust(eo_obj, &ev3.canvas.x, &ev3.canvas.y);
             _evas_event_havemap_adjust(eo_obj, obj, &ev3.canvas.x, &ev3.canvas.y, obj->mouse_grabbed);
             /* if its not in the old list of ins send an enter event */
             if (!eina_list_data_find(e->pointer.object.in, eo_obj))
               {
                  if (!obj->mouse_in)
                    {
                       obj->mouse_in = 1;
                       if (!e->is_frozen)
                         evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_IN, &ev3, event_id2);
                    }
               }
             if (e->delete_me) break;
          }
        if (e->pointer.mouse_grabbed == 0)
          {
             /* free our old list of ins */
             eina_list_free(e->pointer.object.in);
             /* and set up the new one */
             e->pointer.object.in = ins;
          }
        else
          {
             /* free our cur ins */
             eina_list_free(ins);
          }
        _evas_post_event_callback_call(eo_e, e);
        if (ev.dev) _evas_device_unref(ev.dev);
     }
   _evas_unwalk(e); 
   return;
nogrep:
     {
        Eina_List *ins = NULL;
        Eina_List *newin = NULL;
        Eina_List *l, *copy, *lst = NULL;
        Evas_Event_Mouse_Move ev;
        Evas_Event_Mouse_Out ev2;
        Evas_Event_Mouse_In ev3;
        Evas_Object *eo_obj, *eo_below_obj;
        int event_id = 0, event_id2 = 0;
        int norep = 0, breaknext = 0;

        _evas_object_event_new();

        event_id = _evas_event_counter;
        ev.buttons = e->pointer.button;
        ev.cur.output.x = e->pointer.x;
        ev.cur.output.y = e->pointer.y;
        ev.cur.canvas.x = e->pointer.x;
        ev.cur.canvas.y = e->pointer.y;
        ev.prev.output.x = px;
        ev.prev.output.y = py;
        ev.prev.canvas.x = px;
        ev.prev.canvas.y = py;
        ev.data = (void *)data;
        ev.modifiers = &(e->modifiers);
        ev.locks = &(e->locks);
        ev.timestamp = timestamp;
        ev.event_flags = e->default_event_flags;
        ev.dev = _evas_device_top_get(eo_e);
        if (ev.dev) _evas_device_ref(ev.dev);
        
        ev2.buttons = e->pointer.button;
        ev2.output.x = e->pointer.x;
        ev2.output.y = e->pointer.y;
        ev2.canvas.x = e->pointer.x;
        ev2.canvas.y = e->pointer.y;
        ev2.data = (void *)data;
        ev2.modifiers = &(e->modifiers);
        ev2.locks = &(e->locks);
        ev2.timestamp = timestamp;
        ev2.event_flags = e->default_event_flags;
        ev2.dev = ev.dev;
        
        ev3.buttons = e->pointer.button;
        ev3.output.x = e->pointer.x;
        ev3.output.y = e->pointer.y;
        ev3.canvas.x = e->pointer.x;
        ev3.canvas.y = e->pointer.y;
        ev3.data = (void *)data;
        ev3.modifiers = &(e->modifiers);
        ev3.locks = &(e->locks);
        ev3.timestamp = timestamp;
        ev3.event_flags = e->default_event_flags;
        ev3.dev = ev.dev;

        /* go thru old list of in objects */
        copy = evas_event_list_copy(e->pointer.object.in);
        EINA_LIST_FOREACH(copy, l, eo_obj)
          {
             if (breaknext)
               {
                  lst = l;
                  break;
               }
             if (eo_obj == nogrep_obj) breaknext = 1;
          }

        /* get all new in objects */
        eo_below_obj = evas_object_below_get(nogrep_obj);
        if (eo_below_obj)
          {
             Evas_Object_Protected_Data *below_obj = eo_data_get(eo_below_obj, EVAS_OBJ_CLASS);
             ins = _evas_event_object_list_raw_in_get(eo_e, NULL,
                                                   EINA_INLIST_GET(below_obj), NULL,
                                                   e->pointer.x, e->pointer.y,
                                                   &norep);
          }
        EINA_LIST_FOREACH(copy, l, eo_obj)
          {
             newin = eina_list_append(newin, eo_obj);
             if (eo_obj == nogrep_obj) break;
          }
        EINA_LIST_FOREACH(ins, l, eo_obj)
          {
             newin = eina_list_append(newin, eo_obj);
          }

        EINA_LIST_FOREACH(lst, l, eo_obj)
          {
             Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
             /* if its under the pointer and its visible and its in the new */
             /* in list */
             // FIXME: i don't think we need this
             //	     evas_object_clip_recalc(eo_obj);
             if ((!e->is_frozen) &&
                 evas_object_is_in_output_rect(eo_obj, obj, x, y, 1, 1) &&
                 (evas_object_clippers_is_visible(eo_obj, obj) ||
                  obj->mouse_grabbed) &&
                 eina_list_data_find(newin, eo_obj) &&
                 (!evas_event_passes_through(eo_obj, obj)) &&
                 (!evas_event_freezes_through(eo_obj, obj)) &&
                 (!obj->clip.clipees) &&
                 ((!obj->precise_is_inside) || evas_object_is_inside(eo_obj, obj, x, y))
                )
               {
                  if ((px != x) || (py != y))
                    {
                       ev.cur.canvas.x = e->pointer.x;
                       ev.cur.canvas.y = e->pointer.y;
                       _evas_event_framespace_adjust(eo_obj, &ev.cur.canvas.x, &ev.cur.canvas.y);
                       _evas_event_havemap_adjust(eo_obj, obj, &ev.cur.canvas.x, &ev.cur.canvas.y, obj->mouse_grabbed);
                       evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_MOVE, &ev, event_id);
                    }
               }
             /* otherwise it has left the object */
             else
               {
                  if (obj->mouse_in)
                    {
                       obj->mouse_in = 0;
                       ev2.canvas.x = e->pointer.x;
                       ev2.canvas.y = e->pointer.y;
                       _evas_event_framespace_adjust(eo_obj, &ev2.canvas.x, &ev2.canvas.y);
                       _evas_event_havemap_adjust(eo_obj, obj, &ev2.canvas.x, &ev2.canvas.y, obj->mouse_grabbed);
                       if (!e->is_frozen)
                          evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_OUT, &ev2, event_id);
                    }
               }
             if (e->delete_me) break;
          }
        _evas_post_event_callback_call(eo_e, e);

        _evas_object_event_new();

        event_id2 = _evas_event_counter;
        if (copy) copy = eina_list_free(copy);
        /* go thru our current list of ins */
        EINA_LIST_FOREACH(newin, l, eo_obj)
          {
             Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
             ev3.canvas.x = e->pointer.x;
             ev3.canvas.y = e->pointer.y;
             _evas_event_framespace_adjust(eo_obj, &ev3.canvas.x, &ev3.canvas.y);
             _evas_event_havemap_adjust(eo_obj, obj, &ev3.canvas.x, &ev3.canvas.y, obj->mouse_grabbed);
             /* if its not in the old list of ins send an enter event */
             if (!eina_list_data_find(e->pointer.object.in, eo_obj))
               {
                  if (!obj->mouse_in)
                    {
                       obj->mouse_in = 1;
                       if (!e->is_frozen)
                         evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_IN, &ev3, event_id2);
                    }
               }
             if (e->delete_me) break;
          }
        /* free our old list of ins */
        eina_list_free(e->pointer.object.in);
        /* and set up the new one */
        e->pointer.object.in = newin;

        _evas_post_event_callback_call(eo_e, e);
        if (ev.dev) _evas_device_unref(ev.dev);
     }
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_mouse_in(Evas *eo_e, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_mouse_in(timestamp, data));
}

void
_canvas_event_feed_mouse_in(Eo *eo_e, void *_pd, va_list *list)
{
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Eina_List *ins;
   Eina_List *l;
   Evas_Event_Mouse_In ev;
   Evas_Object *eo_obj;
   int event_id = 0;

   e->pointer.inside = 1;

   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   if (e->pointer.mouse_grabbed != 0) return;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   ev.buttons = e->pointer.button;
   ev.output.x = e->pointer.x;
   ev.output.y = e->pointer.y;
   ev.canvas.x = e->pointer.x;
   ev.canvas.y = e->pointer.y;
   ev.data = (void *)data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   _evas_walk(e);
   /* get new list of ins */
   ins = evas_event_objects_event_list(eo_e, NULL, e->pointer.x, e->pointer.y);
   EINA_LIST_FOREACH(ins, l, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        ev.canvas.x = e->pointer.x;
        ev.canvas.y = e->pointer.y;
        _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
        _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);
        if (!eina_list_data_find(e->pointer.object.in, eo_obj))
          {
             if (!obj->mouse_in)
               {
                  obj->mouse_in = 1;
                  if (!e->is_frozen)
                     evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_IN, &ev, event_id);
               }
          }
        if (e->delete_me) break;
     }
   /* free our old list of ins */
   e->pointer.object.in = eina_list_free(e->pointer.object.in);
   /* and set up the new one */
   e->pointer.object.in = ins;
   _evas_post_event_callback_call(eo_e, e);
   evas_event_feed_mouse_move(eo_e, e->pointer.x, e->pointer.y, timestamp, data);
   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_mouse_out(Evas *eo_e, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_mouse_out(timestamp, data));
}

void
_canvas_event_feed_mouse_out(Eo *eo_e, void *_pd, va_list *list)
{
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Evas_Event_Mouse_Out ev;
   int event_id = 0;

   e->pointer.inside = 0;

   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   ev.buttons = e->pointer.button;
   ev.output.x = e->pointer.x;
   ev.output.y = e->pointer.y;
   ev.canvas.x = e->pointer.x;
   ev.canvas.y = e->pointer.y;
   ev.data = (void *)data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   _evas_walk(e);
   /* if our mouse button is inside any objects */
     {
        /* go thru old list of in objects */
        Eina_List *l, *copy;
        Evas_Object *eo_obj;

        copy = evas_event_list_copy(e->pointer.object.in);
        EINA_LIST_FOREACH(copy, l, eo_obj)
          {
             Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
             ev.canvas.x = e->pointer.x;
             ev.canvas.y = e->pointer.y;
             _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
             _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);
             if (obj->mouse_in)
               {
                  obj->mouse_in = 0;
                  if (!obj->delete_me)
                    {
                       if (!e->is_frozen)
                         evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MOUSE_OUT, &ev, event_id);
                    }
                  obj->mouse_grabbed = 0;
               }
             if (e->delete_me) break;
          }
        if (copy) copy = eina_list_free(copy);
        /* free our old list of ins */
        e->pointer.object.in =  eina_list_free(e->pointer.object.in);
        e->pointer.mouse_grabbed = 0;
        _evas_post_event_callback_call(eo_e, e);
     }
   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_multi_down(Evas *eo_e,
                           int d, int x, int y,
                           double rad, double radx, double rady,
                           double pres, double ang,
                           double fx, double fy,
                           Evas_Button_Flags flags, unsigned int timestamp,
                           const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_multi_down(d, x, y, rad, radx, rady, pres, ang, fx, fy, flags, timestamp, data));
}

void
_canvas_event_feed_multi_down(Eo *eo_e, void *_pd, va_list *list)
{
   int d = va_arg(*list, int);
   int x = va_arg(*list, int);
   int y = va_arg(*list, int);
   double rad = va_arg(*list, double);
   double radx = va_arg(*list, double);
   double rady = va_arg(*list, double);
   double pres = va_arg(*list, double);
   double ang = va_arg(*list, double);
   double fx = va_arg(*list, double);
   double fy = va_arg(*list, double);
   Evas_Button_Flags flags = va_arg(*list, Evas_Button_Flags);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Eina_List *l, *copy;
   Evas_Event_Multi_Down ev;
   Evas_Object *eo_obj;
   int addgrab = 0;
   int event_id = 0;

   e->pointer.downs++;
   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   ev.device = d;
   ev.output.x = x;
   ev.output.y = y;
   ev.canvas.x = x;
   ev.canvas.y = y;
   ev.radius = rad;
   ev.radius_x = radx;
   ev.radius_y = rady;
   ev.pressure = pres;
   ev.angle = ang;
   ev.canvas.xsub = fx;
   ev.canvas.ysub = fy;
   ev.data = (void *)data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.flags = flags;
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   _evas_walk(e);
   /* append new touch point to the touch point list */
   _evas_touch_point_append(eo_e, d, x, y);
   if (e->pointer.mouse_grabbed == 0)
     {
        if (e->pointer.downs > 1) addgrab = e->pointer.downs - 1;
     }
   copy = evas_event_list_copy(e->pointer.object.in);
   EINA_LIST_FOREACH(copy, l, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        if (obj->pointer_mode != EVAS_OBJECT_POINTER_MODE_NOGRAB)
          {
             obj->mouse_grabbed += addgrab + 1;
             e->pointer.mouse_grabbed += addgrab + 1;
          }
     }
   EINA_LIST_FOREACH(copy, l, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        ev.canvas.x = x;
        ev.canvas.y = y;
        ev.canvas.xsub = fx;
        ev.canvas.ysub = fy;
        _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
        _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);
        if (x != ev.canvas.x)
          ev.canvas.xsub = ev.canvas.x; // fixme - lost precision
        if (y != ev.canvas.y)
          ev.canvas.ysub = ev.canvas.y; // fixme - lost precision
        if (!e->is_frozen)
          evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MULTI_DOWN, &ev, event_id);
        if (e->delete_me) break;
     }
   if (copy) eina_list_free(copy);
   _evas_post_event_callback_call(eo_e, e);
   /* update touch point's state to EVAS_TOUCH_POINT_STILL */
   _evas_touch_point_update(eo_e, d, x, y, EVAS_TOUCH_POINT_STILL);
   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_multi_up(Evas *eo_e,
                         int d, int x, int y,
                         double rad, double radx, double rady,
                         double pres, double ang,
                         double fx, double fy,
                         Evas_Button_Flags flags, unsigned int timestamp,
                         const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_multi_up(d, x, y, rad, radx, rady, pres, ang, fx, fy, flags, timestamp, data));
}

void
_canvas_event_feed_multi_up(Eo *eo_e, void *_pd, va_list *list)
{
   int d = va_arg(*list, int);
   int x = va_arg(*list, int);
   int y = va_arg(*list, int);
   double rad = va_arg(*list, double);
   double radx = va_arg(*list, double);
   double rady = va_arg(*list, double);
   double pres = va_arg(*list, double);
   double ang = va_arg(*list, double);
   double fx = va_arg(*list, double);
   double fy = va_arg(*list, double);
   Evas_Button_Flags flags = va_arg(*list, Evas_Button_Flags);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Eina_List *l, *copy;
   Evas_Event_Multi_Up ev;
   Evas_Object *eo_obj;
   int event_id = 0;

   if (e->pointer.downs <= 0) return;
   e->pointer.downs--;
   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   ev.device = d;
   ev.output.x = x;
   ev.output.y = y;
   ev.canvas.x = x;
   ev.canvas.y = y;
   ev.radius = rad;
   ev.radius_x = radx;
   ev.radius_y = rady;
   ev.pressure = pres;
   ev.angle = ang;
   ev.canvas.xsub = fx;
   ev.canvas.ysub = fy;
   ev.data = (void *)data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.flags = flags;
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   _evas_walk(e);
   /* update released touch point */
   _evas_touch_point_update(eo_e, d, x, y, EVAS_TOUCH_POINT_UP);
   copy = evas_event_list_copy(e->pointer.object.in);
   EINA_LIST_FOREACH(copy, l, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        ev.canvas.x = x;
        ev.canvas.y = y;
        ev.canvas.xsub = fx;
        ev.canvas.ysub = fy;
        _evas_event_framespace_adjust(eo_obj, &ev.canvas.x, &ev.canvas.y);
        _evas_event_havemap_adjust(eo_obj, obj, &ev.canvas.x, &ev.canvas.y, obj->mouse_grabbed);
        if (x != ev.canvas.x)
          ev.canvas.xsub = ev.canvas.x; // fixme - lost precision
        if (y != ev.canvas.y)
          ev.canvas.ysub = ev.canvas.y; // fixme - lost precision
        if ((obj->pointer_mode != EVAS_OBJECT_POINTER_MODE_NOGRAB) &&
            (obj->mouse_grabbed > 0))
          {
             obj->mouse_grabbed--;
             e->pointer.mouse_grabbed--;
          }
        if (!e->is_frozen)
          evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MULTI_UP, &ev, event_id);
        if (e->delete_me) break;
     }
   if (copy) copy = eina_list_free(copy);
   if ((e->pointer.mouse_grabbed == 0) && !_post_up_handle(eo_e, timestamp, data))
      _evas_post_event_callback_call(eo_e, e);
   /* remove released touch point from the touch point list */
   _evas_touch_point_remove(eo_e, d);
   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_multi_move(Evas *eo_e,
                           int d, int x, int y,
                           double rad, double radx, double rady,
                           double pres, double ang,
                           double fx, double fy,
                           unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   eo_do(eo_e, evas_canvas_event_feed_multi_move(d, x, y, rad, radx, rady, pres, ang, fx, fy, timestamp, data));
}

void
_canvas_event_feed_multi_move(Eo *eo_e, void *_pd, va_list *list)
{
   int d = va_arg(*list, int);
   int x = va_arg(*list, int);
   int y = va_arg(*list, int);
   double rad = va_arg(*list, double);
   double radx = va_arg(*list, double);
   double rady = va_arg(*list, double);
   double pres = va_arg(*list, double);
   double ang = va_arg(*list, double);
   double fx = va_arg(*list, double);
   double fy = va_arg(*list, double);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;

   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   if ((!e->pointer.inside) && (e->pointer.mouse_grabbed == 0)) return;

   _evas_walk(e);
   /* update moved touch point */
   _evas_touch_point_update(eo_e, d, x, y, EVAS_TOUCH_POINT_MOVE);
   /* if our mouse button is grabbed to any objects */
   if (e->pointer.mouse_grabbed > 0)
     {
        /* go thru old list of in objects */
        Eina_List *l, *copy;
        Evas_Event_Multi_Move ev;
        Evas_Object *eo_obj;
        int event_id = 0;

        _evas_object_event_new();

        event_id = _evas_event_counter;
        ev.device = d;
        ev.cur.output.x = x;
        ev.cur.output.y = y;
        ev.cur.canvas.x = x;
        ev.cur.canvas.y = y;
        ev.radius = rad;
        ev.radius_x = radx;
        ev.radius_y = rady;
        ev.pressure = pres;
        ev.angle = ang;
        ev.cur.canvas.xsub = fx;
        ev.cur.canvas.ysub = fy;
        ev.data = (void *)data;
        ev.modifiers = &(e->modifiers);
        ev.locks = &(e->locks);
        ev.timestamp = timestamp;
        ev.event_flags = e->default_event_flags;
        ev.dev = _evas_device_top_get(eo_e);
        if (ev.dev) _evas_device_ref(ev.dev);
        
        copy = evas_event_list_copy(e->pointer.object.in);
        EINA_LIST_FOREACH(copy, l, eo_obj)
          {
             Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
             if ((!e->is_frozen) &&
                 (evas_object_clippers_is_visible(eo_obj, obj) || obj->mouse_grabbed) &&
                 (!evas_event_passes_through(eo_obj, obj)) &&
                 (!evas_event_freezes_through(eo_obj, obj)) &&
                 (!obj->clip.clipees))
               {
                  ev.cur.canvas.x = x;
                  ev.cur.canvas.y = y;
                  ev.cur.canvas.xsub = fx;
                  ev.cur.canvas.ysub = fy;
                  _evas_event_framespace_adjust(eo_obj, &ev.cur.canvas.x, &ev.cur.canvas.y);
                  _evas_event_havemap_adjust(eo_obj, obj, &ev.cur.canvas.x, &ev.cur.canvas.y, obj->mouse_grabbed);
                  if (x != ev.cur.canvas.x)
                    ev.cur.canvas.xsub = ev.cur.canvas.x; // fixme - lost precision
                  if (y != ev.cur.canvas.y)
                    ev.cur.canvas.ysub = ev.cur.canvas.y; // fixme - lost precision
                    evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MULTI_MOVE, &ev, event_id);
               }
             if (e->delete_me) break;
          }
        _evas_post_event_callback_call(eo_e, e);
        if (ev.dev) _evas_device_unref(ev.dev);
     }
   else
     {
        Eina_List *ins;
        Eina_List *l, *copy;
        Evas_Event_Multi_Move ev;
        Evas_Object *eo_obj;
        int event_id = 0;

        _evas_object_event_new();

        event_id = _evas_event_counter;
        ev.device = d;
        ev.cur.output.x = x;
        ev.cur.output.y = y;
        ev.cur.canvas.x = x;
        ev.cur.canvas.y = y;
        ev.radius = rad;
        ev.radius_x = radx;
        ev.radius_y = rady;
        ev.pressure = pres;
        ev.angle = ang;
        ev.cur.canvas.xsub = fx;
        ev.cur.canvas.ysub = fy;
        ev.data = (void *)data;
        ev.modifiers = &(e->modifiers);
        ev.locks = &(e->locks);
        ev.timestamp = timestamp;
        ev.event_flags = e->default_event_flags;
        ev.dev = _evas_device_top_get(eo_e);
        if (ev.dev) _evas_device_ref(ev.dev);
        
        /* get all new in objects */
        ins = evas_event_objects_event_list(eo_e, NULL, x, y);
        /* go thru old list of in objects */
        copy = evas_event_list_copy(e->pointer.object.in);
        EINA_LIST_FOREACH(copy, l, eo_obj)
          {
             Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
             /* if its under the pointer and its visible and its in the new */
             /* in list */
             // FIXME: i don't think we need this
             //	     evas_object_clip_recalc(eo_obj);
             if ((!e->is_frozen) &&
                 evas_object_is_in_output_rect(eo_obj, obj, x, y, 1, 1) &&
                 (evas_object_clippers_is_visible(eo_obj, obj) ||
                  obj->mouse_grabbed) &&
                 eina_list_data_find(ins, eo_obj) &&
                 (!evas_event_passes_through(eo_obj, obj)) &&
                 (!evas_event_freezes_through(eo_obj, obj)) &&
                 (!obj->clip.clipees) &&
                 ((!obj->precise_is_inside) || evas_object_is_inside(eo_obj, obj, x, y))
                )
               {
                  ev.cur.canvas.x = x;
                  ev.cur.canvas.y = y;
                  ev.cur.canvas.xsub = fx;
                  ev.cur.canvas.ysub = fy;
                  _evas_event_framespace_adjust(eo_obj, &ev.cur.canvas.x, &ev.cur.canvas.y);
                  _evas_event_havemap_adjust(eo_obj, obj, &ev.cur.canvas.x, &ev.cur.canvas.y, obj->mouse_grabbed);
                  if (x != ev.cur.canvas.x)
                    ev.cur.canvas.xsub = ev.cur.canvas.x; // fixme - lost precision
                  if (y != ev.cur.canvas.y)
                    ev.cur.canvas.ysub = ev.cur.canvas.y; // fixme - lost precision
                    evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_MULTI_MOVE, &ev, event_id);
               }
             if (e->delete_me) break;
          }
        if (copy) copy = eina_list_free(copy);
        if (e->pointer.mouse_grabbed == 0)
          {
             /* free our old list of ins */
             eina_list_free(e->pointer.object.in);
             /* and set up the new one */
             e->pointer.object.in = ins;
          }
        else
          {
             /* free our cur ins */
             eina_list_free(ins);
          }
        _evas_post_event_callback_call(eo_e, e);
        if (ev.dev) _evas_device_unref(ev.dev);
     }
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_key_down(Evas *eo_e, const char *keyname, const char *key, const char *string, const char *compose, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_e, evas_canvas_event_feed_key_down(keyname, key, string, compose, timestamp, data));
}

void
_canvas_event_feed_key_down(Eo *eo_e, void *_pd, va_list *list)
{
   const char *keyname = va_arg(*list, const char *);
   const char *key = va_arg(*list, const char *);
   const char *string = va_arg(*list, const char *);
   const char *compose = va_arg(*list, const char *);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   int event_id = 0;
   Evas_Public_Data *e = _pd;

   if (!keyname) return;
   if (e->is_frozen) return;
   e->last_timestamp = timestamp;
   _evas_walk(e);

   Evas_Event_Key_Down ev;
   Eina_Bool exclusive;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   exclusive = EINA_FALSE;
   ev.keyname = (char *)keyname;
   ev.data = (void *)data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.key = key;
   ev.string = string;
   ev.compose = compose;
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   if (e->grabs)
     {
        Eina_List *l;
        Evas_Key_Grab *g;

        e->walking_grabs++;
        EINA_LIST_FOREACH(e->grabs, l, g)
          {
             if (g->just_added)
               {
                  g->just_added = EINA_FALSE;
                  continue;
               }
             if (g->delete_me) continue;
             if (((e->modifiers.mask & g->modifiers) ||
                  (g->modifiers == e->modifiers.mask)) &&
                 (!strcmp(keyname, g->keyname)))
               {
                  if (!(e->modifiers.mask & g->not_modifiers))
                    {
                       Evas_Object_Protected_Data *object_obj = eo_data_get(g->object, EVAS_OBJ_CLASS);
                       if (!e->is_frozen &&
                           !evas_event_freezes_through(g->object, object_obj))
                         evas_object_event_callback_call(g->object, object_obj,
                                                         EVAS_CALLBACK_KEY_DOWN,
                                                         &ev, event_id);
                       if (g->exclusive) exclusive = EINA_TRUE;
                    }
               }
             if (e->delete_me) break;
          }
        e->walking_grabs--;
        if (e->walking_grabs <= 0)
          {
             while (e->delete_grabs > 0)
               {
                  e->delete_grabs--;
                  for (l = e->grabs; l;)
                    {
                       g = eina_list_data_get(l);
                       l = eina_list_next(l);
                       if (g->delete_me)
                         {
                            Evas_Object_Protected_Data *g_object_obj = eo_data_get(g->object, EVAS_OBJ_CLASS);
                            evas_key_grab_free(g->object, g_object_obj, g->keyname,
                                               g->modifiers, g->not_modifiers);
                         }
                    }
               }
          }
     }
   if ((e->focused) && (!exclusive))
     {
        Evas_Object_Protected_Data *focused_obj = eo_data_get(e->focused, EVAS_OBJ_CLASS);
        if (!e->is_frozen && !evas_event_freezes_through(e->focused, focused_obj))
          evas_object_event_callback_call(e->focused, focused_obj, EVAS_CALLBACK_KEY_DOWN,
                                          &ev, event_id);
     }
   _evas_post_event_callback_call(eo_e, e);
   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_key_up(Evas *eo_e, const char *keyname, const char *key, const char *string, const char *compose, unsigned int timestamp, const void *data)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_e, evas_canvas_event_feed_key_up(keyname, key, string, compose, timestamp, data));
}

void
_canvas_event_feed_key_up(Eo *eo_e, void *_pd, va_list *list)
{
   const char *keyname = va_arg(*list, const char *);
   const char *key = va_arg(*list, const char *);
   const char *string = va_arg(*list, const char *);
   const char *compose = va_arg(*list, const char *);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   int event_id = 0;
   Evas_Public_Data *e = _pd;
   if (!keyname) return;
   if (e->is_frozen) return;
   e->last_timestamp = timestamp;
   _evas_walk(e);

   Evas_Event_Key_Up ev;
   Eina_Bool exclusive;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   exclusive = EINA_FALSE;
   ev.keyname = (char *)keyname;
   ev.data = (void *)data;
   ev.modifiers = &(e->modifiers);
   ev.locks = &(e->locks);
   ev.key = key;
   ev.string = string;
   ev.compose = compose;
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   if (e->grabs)
     {
        Eina_List *l;
        Evas_Key_Grab *g;

        e->walking_grabs++;
        EINA_LIST_FOREACH(e->grabs, l, g)
          {
             if (g->just_added)
               {
                  g->just_added = EINA_FALSE;
                  continue;
               }
             if (g->delete_me) continue;
             if (((e->modifiers.mask & g->modifiers) ||
                  (g->modifiers == e->modifiers.mask)) &&
                 (!((e->modifiers.mask & g->not_modifiers) ||
                    (g->not_modifiers == ~e->modifiers.mask))) &&
                 (!strcmp(keyname, g->keyname)))
               {
                  Evas_Object_Protected_Data *object_obj = eo_data_get(g->object, EVAS_OBJ_CLASS);
                  if (!e->is_frozen &&
                        !evas_event_freezes_through(g->object, object_obj))
                    evas_object_event_callback_call(g->object, object_obj,
                                                    EVAS_CALLBACK_KEY_UP, &ev, event_id);
                  if (g->exclusive) exclusive = EINA_TRUE;
               }
             if (e->delete_me) break;
          }
        e->walking_grabs--;
        if (e->walking_grabs <= 0)
          {
             while (e->delete_grabs > 0)
               {
                  Eina_List *ll, *l_next;
                  Evas_Key_Grab *gr;

                  e->delete_grabs--;
                  EINA_LIST_FOREACH_SAFE(e->grabs, ll, l_next, gr)
                    {
                       if (gr->delete_me)
                         {
                            Evas_Object_Protected_Data *gr_object_obj =
                               eo_data_get(gr->object, EVAS_OBJ_CLASS);
                            evas_key_grab_free(gr->object, gr_object_obj, gr->keyname,
                                            gr->modifiers, gr->not_modifiers);
                         }
                    }
               }
          }
     }
   if ((e->focused) && (!exclusive))
     {
        Evas_Object_Protected_Data *focused_obj = eo_data_get(e->focused, EVAS_OBJ_CLASS);
        if (!e->is_frozen && !evas_event_freezes_through(e->focused, focused_obj))
          evas_object_event_callback_call(e->focused, focused_obj, EVAS_CALLBACK_KEY_UP,
                                          &ev, event_id);
     }
   _evas_post_event_callback_call(eo_e, e);
   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
}

EAPI void
evas_event_feed_hold(Evas *eo_e, int hold, unsigned int timestamp, const void *data)
{
   eo_do(eo_e, evas_canvas_event_feed_hold(hold, timestamp, data));
}

void
_canvas_event_feed_hold(Eo *eo_e, void *_pd, va_list *list)
{
   int hold = va_arg(*list, int);
   unsigned int timestamp = va_arg(*list, unsigned int);
   const void *data = va_arg(*list, const void *);

   Evas_Public_Data *e = _pd;
   Eina_List *l, *copy;
   Evas_Event_Hold ev;
   Evas_Object *eo_obj;
   int event_id = 0;

   if (e->is_frozen) return;
   e->last_timestamp = timestamp;

   _evas_object_event_new();

   event_id = _evas_event_counter;
   ev.hold = hold;
   ev.data = (void *)data;
   ev.timestamp = timestamp;
   ev.event_flags = e->default_event_flags;
   ev.dev = _evas_device_top_get(eo_e);
   if (ev.dev) _evas_device_ref(ev.dev);
   
   _evas_walk(e);
   copy = evas_event_list_copy(e->pointer.object.in);
   EINA_LIST_FOREACH(copy, l, eo_obj)
     {
        Evas_Object_Protected_Data *obj = eo_data_get(eo_obj, EVAS_OBJ_CLASS);
        if ((!e->is_frozen) && !evas_event_freezes_through(eo_obj, obj))
          evas_object_event_callback_call(eo_obj, obj, EVAS_CALLBACK_HOLD, &ev, event_id);
        if (e->delete_me) break;
     }
   if (copy) copy = eina_list_free(copy);
   _evas_post_event_callback_call(eo_e, e);
   if (ev.dev) _evas_device_unref(ev.dev);
   _evas_unwalk(e);
   _evas_object_event_new();
}

EAPI void
evas_object_freeze_events_set(Evas_Object *eo_obj, Eina_Bool freeze)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_obj, evas_obj_freeze_events_set(freeze));
}

void
_freeze_events_set(Eo *eo_obj, void *_pd, va_list *list)
{
   Eina_Bool freeze = va_arg(*list, int);
   Evas_Object_Protected_Data *obj = _pd;
   freeze = !!freeze;
   if (obj->freeze_events == freeze) return;
   obj->freeze_events = freeze;
   evas_object_smart_member_cache_invalidate(eo_obj, EINA_FALSE, EINA_TRUE);
}

EAPI Eina_Bool
evas_object_freeze_events_get(const Evas_Object *eo_obj)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EINA_FALSE;
   MAGIC_CHECK_END();
   Eina_Bool freeze_events = EINA_FALSE;
   eo_do((Eo *)eo_obj, evas_obj_freeze_events_get(&freeze_events));
   return freeze_events;
}

void
_freeze_events_get(Eo *eo_obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *freeze_events = va_arg(*list, Eina_Bool *);
   const Evas_Object_Protected_Data *obj = _pd;
   if (freeze_events) *freeze_events = obj->freeze_events;
}

EAPI void
evas_object_pass_events_set(Evas_Object *eo_obj, Eina_Bool pass)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_obj, evas_obj_pass_events_set(pass));
}

void
_pass_events_set(Eo *eo_obj, void *_pd, va_list *list)
{
   Eina_Bool pass = va_arg(*list, int);
   Evas_Object_Protected_Data *obj = _pd;
   pass = !!pass;
   if (obj->pass_events == pass) return;
   obj->pass_events = pass;
   evas_object_smart_member_cache_invalidate(eo_obj, EINA_TRUE, EINA_FALSE);
   if (evas_object_is_in_output_rect(eo_obj, obj,
                                     obj->layer->evas->pointer.x,
                                     obj->layer->evas->pointer.y, 1, 1) &&
       ((!obj->precise_is_inside) ||
        (evas_object_is_inside(eo_obj, obj,
                               obj->layer->evas->pointer.x,
                               obj->layer->evas->pointer.y))))
     evas_event_feed_mouse_move(obj->layer->evas->evas,
                                obj->layer->evas->pointer.x,
                                obj->layer->evas->pointer.y,
                                obj->layer->evas->last_timestamp,
                                NULL);
}

EAPI Eina_Bool
evas_object_pass_events_get(const Evas_Object *eo_obj)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EINA_FALSE;
   MAGIC_CHECK_END();
   Eina_Bool pass_events = EINA_FALSE;
   eo_do((Eo *)eo_obj, evas_obj_pass_events_get(&pass_events));
   return pass_events;
}

void
_pass_events_get(Eo *eo_obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *pass_events = va_arg(*list, Eina_Bool *);
   const Evas_Object_Protected_Data *obj = _pd;
   if (pass_events) *pass_events = obj->pass_events;
}

EAPI void
evas_object_repeat_events_set(Evas_Object *eo_obj, Eina_Bool repeat)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_obj, evas_obj_repeat_events_set(repeat));
}

void
_repeat_events_set(Eo *eo_obj, void *_pd, va_list *list)
{
   Eina_Bool repeat = va_arg(*list, int);
   Evas_Object_Protected_Data *obj = _pd;
   repeat = !!repeat;
   if (obj->repeat_events == repeat) return;
   obj->repeat_events = repeat;
   if (evas_object_is_in_output_rect(eo_obj, obj,
                                     obj->layer->evas->pointer.x,
                                     obj->layer->evas->pointer.y, 1, 1) &&
       ((!obj->precise_is_inside) ||
        (evas_object_is_inside(eo_obj, obj,
                               obj->layer->evas->pointer.x,
                               obj->layer->evas->pointer.y))))
     evas_event_feed_mouse_move(obj->layer->evas->evas,
                                obj->layer->evas->pointer.x,
                                obj->layer->evas->pointer.y,
                                obj->layer->evas->last_timestamp,
                                NULL);
}

EAPI Eina_Bool
evas_object_repeat_events_get(const Evas_Object *eo_obj)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EINA_FALSE;
   MAGIC_CHECK_END();
   Eina_Bool repeat_events = EINA_FALSE;
   eo_do((Eo *)eo_obj, evas_obj_repeat_events_get(&repeat_events));
   return repeat_events;
}

void
_repeat_events_get(Eo *eo_obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *repeat_events = va_arg(*list, Eina_Bool *);
   const Evas_Object_Protected_Data *obj = _pd;
   if (repeat_events) *repeat_events = obj->repeat_events;
}

EAPI void
evas_object_propagate_events_set(Evas_Object *eo_obj, Eina_Bool prop)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_obj, evas_obj_propagate_events_set(prop));
}

void
_propagate_events_set(Eo *eo_obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool prop = va_arg(*list, int);
   Evas_Object_Protected_Data *obj = _pd;
   obj->no_propagate = !prop;
}

EAPI Eina_Bool
evas_object_propagate_events_get(const Evas_Object *eo_obj)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EINA_FALSE;
   MAGIC_CHECK_END();
   Eina_Bool no_propagate = EINA_FALSE;
   eo_do((Eo *)eo_obj, evas_obj_propagate_events_get(&no_propagate));
   return no_propagate;
}

void
_propagate_events_get(Eo *eo_obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *no_propagate = va_arg(*list, Eina_Bool *);
   const Evas_Object_Protected_Data *obj = _pd;
   if (no_propagate) *no_propagate = !(obj->no_propagate);
}

EAPI void
evas_object_pointer_mode_set(Evas_Object *eo_obj, Evas_Object_Pointer_Mode setting)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();
   eo_do(eo_obj, evas_obj_pointer_mode_set(setting));
}

void
_pointer_mode_set(Eo *eo_obj EINA_UNUSED, void *_pd, va_list *list)
{
   Evas_Object_Pointer_Mode setting = va_arg(*list, Evas_Object_Pointer_Mode);
   Evas_Object_Protected_Data *obj = _pd;
   obj->pointer_mode = setting;
}

EAPI Evas_Object_Pointer_Mode
evas_object_pointer_mode_get(const Evas_Object *eo_obj)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EVAS_OBJECT_POINTER_MODE_AUTOGRAB;
   MAGIC_CHECK_END();
   Evas_Object_Pointer_Mode setting = EVAS_OBJECT_POINTER_MODE_AUTOGRAB;
   eo_do((Eo *)eo_obj, evas_obj_pointer_mode_get(&setting));
   return setting;
}

void
_pointer_mode_get(Eo *eo_obj EINA_UNUSED, void *_pd, va_list *list)
{
   Evas_Object_Pointer_Mode *setting = va_arg(*list, Evas_Object_Pointer_Mode *);
   const Evas_Object_Protected_Data *obj = _pd;
   if (setting) *setting = obj->pointer_mode;
}

EAPI void
evas_event_refeed_event(Evas *eo_e, void *event_copy, Evas_Callback_Type event_type)
{
   eo_do(eo_e, evas_canvas_event_refeed_event(event_copy, event_type));
}

void
_canvas_event_refeed_event(Eo *eo_e, void *_pd EINA_UNUSED, va_list *list)
{
   void *event_copy = va_arg(*list, void *);
   Evas_Callback_Type event_type = va_arg(*list, Evas_Callback_Type);

   if (!event_copy) return;

   switch (event_type)
     {
      case EVAS_CALLBACK_MOUSE_IN:
          {
             Evas_Event_Mouse_In *ev = event_copy;
             evas_event_feed_mouse_in(eo_e, ev->timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_MOUSE_OUT:
          {
             Evas_Event_Mouse_Out *ev = event_copy;
             evas_event_feed_mouse_out(eo_e, ev->timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_MOUSE_DOWN:
          {
             Evas_Event_Mouse_Down *ev = event_copy;
             evas_event_feed_mouse_down(eo_e, ev->button, ev->flags, ev-> timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_MOUSE_UP:
          {
             Evas_Event_Mouse_Up *ev = event_copy;
             evas_event_feed_mouse_up(eo_e, ev->button, ev->flags, ev-> timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_MOUSE_MOVE:
          {
             Evas_Event_Mouse_Move *ev = event_copy;
             evas_event_feed_mouse_move(eo_e, ev->cur.canvas.x, ev->cur.canvas.y, ev->timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_MOUSE_WHEEL:
          {
             Evas_Event_Mouse_Wheel *ev = event_copy;
             evas_event_feed_mouse_wheel(eo_e, ev->direction, ev-> z, ev->timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_MULTI_DOWN:
          {
             Evas_Event_Multi_Down *ev = event_copy;
             evas_event_feed_multi_down(eo_e, ev->device, ev->canvas.x, ev->canvas.y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, ev->canvas.xsub, ev->canvas.ysub, ev->flags, ev->timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_MULTI_UP:
          {
             Evas_Event_Multi_Up *ev = event_copy;
             evas_event_feed_multi_up(eo_e, ev->device, ev->canvas.x, ev->canvas.y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, ev->canvas.xsub, ev->canvas.ysub, ev->flags, ev->timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_MULTI_MOVE:
          {
             Evas_Event_Multi_Move *ev = event_copy;
             evas_event_feed_multi_move(eo_e, ev->device, ev->cur.canvas.x, ev->cur.canvas.y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, ev->cur.canvas.xsub, ev->cur.canvas.ysub, ev->timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_KEY_DOWN:
          {
             Evas_Event_Key_Down *ev = event_copy;
             evas_event_feed_key_down(eo_e, ev->keyname, ev->key, ev->string, ev->compose, ev->timestamp, ev->data);
             break;
          }
      case EVAS_CALLBACK_KEY_UP:
          {
             Evas_Event_Key_Up *ev = event_copy;
             evas_event_feed_key_up(eo_e, ev->keyname, ev->key, ev->string, ev->compose, ev->timestamp, ev->data);
             break;
          }
      default: /* All non-input events are not handeled */
        break;
     }
}

EAPI int
evas_event_down_count_get(const Evas *eo_e)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return 0;
   MAGIC_CHECK_END();
   int ret = 0;
   eo_do((Eo *)eo_e, evas_canvas_event_down_count_get(&ret));
   return ret;
}

void
_canvas_event_down_count_get(Eo *eo_e EINA_UNUSED, void *_pd, va_list *list)
{
   int *ret = va_arg(*list, int *);
   const Evas_Public_Data *e = _pd;
   *ret = e->pointer.downs;
}
