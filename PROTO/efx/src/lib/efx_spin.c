#include "efx_private.h"

typedef struct Efx_Spin_Data
{
   EFX *e;
   Evas_Map *map;
   Ecore_Animator *anim;
   long dps;
   double start;
   unsigned int frame;
} Efx_Spin_Data;


static void
_obj_del(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   EFX *e;
   Efx_Spin_Data *esd;

   e = evas_object_data_get(obj, "efx-data");
   if (!e) return; /* bug? */
   esd = e->spin_data;
   if (esd)
     {
        if (esd->anim) ecore_animator_del(esd->anim);
        e->spin_data = NULL;
        free(esd);
     }
   if ((!e->zoom_data) && (!e->rotate_data))
     efx_free(e);
}

static Eina_Bool
_spin_cb(Efx_Spin_Data *esd)
{
   Evas_Map *map;
   double fps;

   fps = 1.0 / ecore_animator_frametime_get();

   map = evas_map_new(4);
   evas_map_smooth_set(map, EINA_FALSE);
   evas_map_util_points_populate_from_object(map, esd->e->obj);
   esd->e->rotate.current = (double)esd->frame * ((double)esd->dps / fps) + esd->start;
   efx_rotate_helper(esd->e, map, esd->e->rotate.current);
   if (esd->e->zoom_data) _efx_zoom_calc(esd->e->zoom_data, map);
/*
   if (esd->frame % (int)fps == 0)
     DBG("frame: %u || rotate: %g", esd->frame, esd->e->rotate.current);
*/
   evas_object_map_set(esd->e->obj, map);
   evas_object_map_enable_set(esd->e->obj, EINA_TRUE);
   evas_map_free(map);
   if (!fmod(esd->e->rotate.current, 360.0)) esd->frame = 0;
   esd->frame++; /* FIXME: this may overflow */

   return EINA_TRUE;
}

static void
_spin_stop(Evas_Object *obj, Eina_Bool reset)
{
   EFX *e;
   Efx_Spin_Data *esd;

   e = evas_object_data_get(obj, "efx-data");
   if ((!e) || (!e->spin_data)) return;
   esd = e->spin_data;
   esd->frame = 0;
   if (reset)
     {
        esd->e->rotate.current = 0;
        efx_rotate_center_init(esd->e, NULL);
        _spin_cb(esd);
        evas_object_event_callback_del_full(obj, EVAS_CALLBACK_FREE, (Evas_Object_Event_Cb)_obj_del, esd);
        _obj_del(esd, NULL, NULL, NULL);
        INF("reset spinning object %p", obj);
     }
   else
     {
        INF("stopped spinning object %p", obj);
        if (esd->anim) ecore_animator_del(esd->anim);
        free(esd);
        e->spin_data = NULL;
     }
}

void
_efx_spin_calc(void *data, Evas_Map *map)
{
   Efx_Spin_Data *esd = data;
   efx_rotate_helper(esd->e, map, esd->e->rotate.current);
}

EAPI Eina_Bool
efx_spin_start(Evas_Object *obj, long dps, const Evas_Point *center)
{
   EFX *e;
   Efx_Spin_Data *esd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(dps, EINA_FALSE);
   /* must stop rotating if object is in motion */
   efx_rotate_stop(obj);
   e = evas_object_data_get(obj, "efx-data");
   if (!e) e = efx_new(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   if (!efx_rotate_center_init(e, center)) return EINA_FALSE;
   esd = e->spin_data;
   if (esd)
     {
        esd->dps = dps;
        esd->start = esd->e->rotate.current;
        if (!esd->anim) esd->anim = ecore_animator_add((Ecore_Task_Cb)_spin_cb, esd);
        if (e->rotate.center)
          INF("spin modified: %p - %s around (%d,%d) || %lddps", obj, (dps > 0) ? "clockwise" : "counter-clockwise",
              e->rotate.center->x, e->rotate.center->y, dps);
        else
          INF("spin modified: %p - %s || %lddps", obj, (dps > 0) ? "clockwise" : "counter-clockwise", dps);
        return EINA_TRUE;
     }
   else
     {
        e->spin_data = esd = calloc(1, sizeof(Efx_Spin_Data));
        EINA_SAFETY_ON_NULL_RETURN_VAL(esd, EINA_FALSE);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE, (Evas_Object_Event_Cb)_obj_del, e->spin_data);
     }

   esd->e = e;
   esd->dps = dps;
   esd->start = e->rotate.current;
   if (e->rotate.center)
     INF("spin: %p - %s around (%d,%d) || %lddps", obj, (dps > 0) ? "clockwise" : "counter-clockwise",
         e->rotate.center->x, e->rotate.center->y, dps);
   else
     INF("spin: %p - %s || %lddps", obj, (dps > 0) ? "clockwise" : "counter-clockwise", dps);
   esd->anim = ecore_animator_add((Ecore_Task_Cb)_spin_cb, esd);
   return EINA_TRUE;
   (void)efx_speed_str;
}

void
efx_spin_reset(Evas_Object *obj)
{
   _spin_stop(obj, EINA_TRUE);
}
void
efx_spin_stop(Evas_Object *obj)
{
   _spin_stop(obj, EINA_FALSE);
}
