#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Efx.h>
#include <Ecore.h>
#include <Ecore_Evas.h>

static Evas *e;
static Eina_List *objs = NULL;

static Eina_Bool _start(Evas_Object *r);

static Evas_Object *
rect_create(void)
{
   Evas_Object *r;
   r = evas_object_rectangle_add(e);
   objs = eina_list_append(objs, r);
   switch (eina_list_count(objs))
     {
      case 1:
        evas_object_color_set(r, 255, 0, 0, 255);
        break;
      case 2:
        evas_object_color_set(r, 0, 255, 0, 255);
        break;
      case 3:
        evas_object_color_set(r, 0, 0, 255, 255);
        break;
      default:
        evas_object_color_set(r, 0, 0, 0, 255);
     }
   evas_object_resize(r, 72, 72);
   evas_object_move(r, 25, 25);
   evas_object_show(r);
   return r;
}

static void
_del(void *data __UNUSED__, Efx_Map_Data *emd __UNUSED__, Evas_Object *r)
{
   objs = eina_list_remove_list(objs, objs);
   evas_object_del(r);
   if (objs) return;
   r = rect_create();
   ecore_timer_add(1.0, (Ecore_Task_Cb)_start, r);
}

static void
_center(void *data __UNUSED__, Efx_Map_Data *emd __UNUSED__, Evas_Object *r)
{
   efx_rotate(r, EFX_EFFECT_SPEED_LINEAR, 360, NULL, 1.5, NULL, NULL);
}

static void
_create4(void *data __UNUSED__, Efx_Map_Data *emd __UNUSED__, Evas_Object *r)
{
   r = rect_create();
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {350, 25}},
     1.0, _center, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_DECELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {203, 203}},
     1.0, NULL, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {550, 550}},
     1.0, _del, NULL);
   efx_queue_run(r);
}

static void
_create3(void *data __UNUSED__, Efx_Map_Data *emd __UNUSED__, Evas_Object *r)
{
   r = rect_create();
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {350, 25}},
     1.0, NULL, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_DECELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {350, 350}},
     1.0, _center, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_DECELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {203, 203}},
     1.0, NULL, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {-100, 550}},
     1.0, _del, NULL);
   efx_queue_run(r);
}

static void
_create2(void *data __UNUSED__, Efx_Map_Data *emd __UNUSED__, Evas_Object *r)
{
   r = rect_create();
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {350, 25}},
     1.0, NULL, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_DECELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {350, 350}},
     1.0, NULL, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {25, 350}},
     1.0, _center, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_DECELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {203, 203}},
     1.0, NULL, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {550, -100}},
     1.0, _del, NULL);
   efx_queue_run(r);
}

static Eina_Bool
_start(Evas_Object *r)
{
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {350, 25}},
     1.0, _create2, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_DECELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {350, 350}},
     1.0, _create3, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {25, 350}},
     1.0, _create4, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_DECELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {25, 25}},
     1.0, _center, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_DECELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {203, 203}},
     1.0, NULL, NULL);
   efx_queue_append(r, EFX_EFFECT_SPEED_ACCELERATE,
     &(Efx_Queued_Effect){EFX_EFFECT_TYPE_MOVE, .effect.movement.point = {-100, -100}},
     1.0, _del, NULL);
   efx_queue_run(r);
   
   return EINA_FALSE;
}

static void
_end(Ecore_Evas *ee __UNUSED__)
{
   ecore_main_loop_quit();
}

int
main(void)
{
   Ecore_Evas *ee;
   Evas_Object *r;

   efx_init();
   ecore_evas_init();
   eina_log_domain_level_set("efx", EINA_LOG_LEVEL_DBG);
   ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 450, 450);
   ecore_evas_callback_delete_request_set(ee, _end);
   ecore_evas_title_set(ee, "queue");
   ecore_evas_show(ee);
   e = ecore_evas_get(ee);
   r = evas_object_rectangle_add(e);
   evas_object_resize(r, 450, 450);
   evas_object_show(r);

   r = rect_create();

   ecore_timer_add(1.0, (Ecore_Task_Cb)_start, r);
   ecore_main_loop_begin();
   return 0;
}
