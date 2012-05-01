/* gcc -O3 -Wall -o fractale-orc fractale-orc.c fractale-julia-orc.c `pkg-config --cflags --libs ecore-evas ecore evas eina orc` */
/*
 * This program display Julia's fractale.
 * Copyright (C) 2010 Vincent Torri
 *                    Issa
 *                    Cedric Bail
 *
 * This program show how to use Ecore_Thread, Eina_F16p16 and Orc in a CPU heavy task.
 * Orc: http://code.entropywave.com/documentation/orc/
 */

#include <stdlib.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>

#include "fractale-julia-orc.h"

#include <assert.h>

#define N 48
#define AREA_SIZE 64

typedef struct
{
   unsigned int ptr[AREA_SIZE][AREA_SIZE];

   Ecore_Thread *ref;

   Evas_Object *target;
   int tw;
   int th;
   int x;
   int y;

   double Ca;
   double Cb;
} Job_Data;

static Eina_List *jobs = NULL;
static Evas_Object *target = NULL;
static double GCa = 0.0, GCb = 0.0;
static double NCa = 0.0, NCb = 0.0;
static Ecore_Idler *idle = NULL;
static Eina_Bool resize = EINA_FALSE;
static Eina_Trash *trash = NULL;

static Ecore_Animator *anim = NULL;
static Eina_List *done = NULL;

static unsigned int jobs_count = 0;
static double start_time = 0.0;


static void
draw_once(Job_Data *d)
{
   unsigned char *pixels;
   void *m;
   unsigned int line;
   unsigned int maxw;
   unsigned int maxh;
   int w, h;

   m = evas_object_image_data_get (d->target, 1);

   evas_object_geometry_get(d->target, NULL, NULL, &w, &h);

   pixels = (unsigned char*)(m) + (d->x + d->y * d->tw) * sizeof (unsigned int);

   maxw = d->x + AREA_SIZE < d->tw ? AREA_SIZE : d->tw - d->x;
   maxh = d->y + AREA_SIZE < d->th ? AREA_SIZE : d->th - d->y;
   for (line = 0; line < maxh; line++, pixels += d->tw * sizeof (unsigned int))
     {
        memcpy(pixels, &d->ptr[line][0], maxw * sizeof (unsigned int));
     }

   evas_object_image_data_set(d->target, m);
   evas_object_image_data_update_add(d->target, d->x, d->y, AREA_SIZE, AREA_SIZE);

   eina_trash_push(&trash, d);

   jobs_count--;

   if (!jobs_count)
     fprintf(stderr, "took: %f\n", ecore_time_get() - start_time);
}

static Eina_Bool
draw_anim(void *data)
{
   Job_Data *d;

   EINA_LIST_FREE(done, d)
     draw_once(d);

   anim = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
draw_end(void *data, Ecore_Thread *thread)
{
   Job_Data *d = data;

   if (!anim) anim = ecore_animator_add(draw_anim, NULL);
   done = eina_list_append(done, d);

   jobs = eina_list_remove(jobs, d->ref);
}

static void
draw_cancel(void *data, Ecore_Thread *thread)
{
   Job_Data *d = data;

   jobs = eina_list_remove(jobs, d->ref);

   eina_trash_push(&trash, data);
}


#define FP Eina_F16p16
#define MUL(a, b) eina_f16p16_mul(a, b)
#define DIV(a, b) eina_f16p16_div(a, b)
#define ADD(a, b) eina_f16p16_add(a, b)
#define SUB(a, b) eina_f16p16_sub(a, b)
#define SCALE(a, b) eina_f16p16_scale(a, b)
#define ITOFP(a) eina_f16p16_int_from(a)
#define DTOFP(a) eina_f16p16_float_from(a)

static void
draw_fractal(void *data, Ecore_Thread *thread)
{
   Job_Data *d = data;
   FP tw, th;
   FP twi, thi;
   FP ca, cb;
   FP f4, f2;
   FP N255, N128;
   FP *Za;
   FP *Zb;
   unsigned int *tmp;
   unsigned int x, y;
   unsigned int i, j;
   int offset;

   /* 1. Init scalar value */
   tw = ITOFP(d->tw);
   th = ITOFP(d->th);
   ca = DTOFP(d->Ca);
   cb = DTOFP(d->Cb);
   N255 = DIV(ITOFP(255), ITOFP(N));
   N128 = DIV(ITOFP(128), ITOFP(N));
   twi = DIV(ITOFP(1), tw);
   thi = DIV(ITOFP(1), th);

   f4 = ITOFP(4);
   f2 = ITOFP(2);

   tmp = alloca(sizeof (int) * AREA_SIZE);
   Za = alloca(sizeof (Eina_F16p16) * AREA_SIZE);
   memset(Za, 0, sizeof (Eina_F16p16) * AREA_SIZE);
   Zb = alloca(sizeof (Eina_F16p16) * AREA_SIZE);
   memset(Zb, 0, sizeof (Eina_F16p16) * AREA_SIZE);

   /* 2. Computing all possible Za */
   x = d->x;
   for (i = 0; i < AREA_SIZE; i++, x++)
     tmp[i] = x;
   _fractal_outerloop_fp_za(Za, tmp, twi, AREA_SIZE);

   /* 3. Computing all possible Zb */
   y = d->y;
   for (i = 0; i < AREA_SIZE; i++, y++)
     tmp[i] = y;
   _fractal_outerloop_fp_zb(Zb, tmp, thi, AREA_SIZE);

   
   
}

static void
_rebuild_thread_list(Evas_Object *obj, int w, int h)
{
  Ecore_Thread *t;
  Job_Data *d;
  int x = 0, y = 0;

  EINA_LIST_FREE(jobs, t)
    ecore_thread_cancel(t);

  EINA_LIST_FREE(done, d)
    eina_trash_push(&trash, d);

  if (anim) ecore_animator_del(anim);
  anim = NULL;

  start_time = ecore_time_get();

  for (x = 0; x < w; x += AREA_SIZE)
    for (y = 0; y < h; y += AREA_SIZE)
      {
         d = calloc(1, sizeof (Job_Data));
         if (!d) continue ;

         d->target = target;
         d->x = x;
         d->y = y;
         d->Ca = GCa;
         d->Cb = GCb;
         d->tw = w;
         d->th = h;

         t = ecore_thread_run(draw_fractal,
                              draw_end,
                              draw_cancel,
                              d);
         if (!t) continue ;
         d->ref = t;
         jobs = eina_list_append(jobs, t);
      }

  jobs_count = eina_list_count(jobs);
}

static void
_cb_delete (Ecore_Evas *ee)
{
   ecore_main_loop_quit();
}

static Eina_Bool
_cb_idle(void *data)
{
   Evas_Object *obj = data;
   int w, h;

   idle = NULL;
   if (GCa == NCa && GCb == NCb && resize == EINA_FALSE) return ECORE_CALLBACK_CANCEL;

   GCa = NCa;
   GCb = NCb;
   resize = EINA_FALSE;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   _rebuild_thread_list(obj, w, h);

   return ECORE_CALLBACK_CANCEL;
}

static void
_cb_resize (Ecore_Evas *ee)
{
   Ecore_Thread *t;
   Job_Data *d;
   int w, h;
   void *m;

   EINA_LIST_FREE(jobs, t)
     ecore_thread_cancel(t);

   EINA_LIST_FREE(done, d)
     eina_trash_push(&trash, d);

   if (anim) ecore_animator_del(anim);
   anim = NULL;

   ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);

   evas_object_image_size_set(target, w, h);
   evas_object_image_fill_set(target, 0, 0, w, h);
   m = evas_object_image_data_get(target, 1);
   memset(m, 0, w * h * 4);
   evas_object_image_data_set(target, m);
   evas_object_image_data_update_add(target, 0, 0, w, h);
   evas_object_resize(target, w, h);

   if (!idle) idle = ecore_idler_add(_cb_idle, target);
   resize = EINA_TRUE;
}

static void
cb_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *event;
   int w, h;

   event = (Evas_Event_Mouse_Move *)event_info;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   NCa = 4 * (double)event->cur.output.x / w - 2;
   NCb = -4 * (double)event->cur.output.y / h + 2;

   if (!idle) idle = ecore_idler_add(_cb_idle, obj);
}

int
main(int argc, char **argv)
{
   Ecore_Evas   *ee;
   Evas         *evas;
   Evas_Object  *o;
   Ecore_Thread *t;
   void *m;
   int           width;
   int           height;

   if (!ecore_evas_init())
     return EXIT_FAILURE;

   width = 480;
   height = 480;

   ee = ecore_evas_new("software_x11", 0, 0, width, height, NULL);
   if (!ee)
     {
        ecore_evas_shutdown();
        return EXIT_FAILURE;
     }

   ecore_evas_callback_delete_request_set(ee, _cb_delete);
   ecore_evas_callback_resize_set(ee, _cb_resize);
   ecore_evas_show(ee);
   ecore_evas_title_set(ee, "ELF and ORC together for Julia");
   evas = ecore_evas_get(ee);

   o = evas_object_image_add(evas);
   evas_object_move(o, 0, 0);

   // fill the 100x100 image with black pixels
   evas_object_image_size_set(o, width, height);
   evas_object_image_fill_set(o, 0, 0, width, height);
   m = evas_object_image_data_get(o, 1);
   memset(m, 0, width * height * 4);
   evas_object_image_data_set(o, m);
   evas_object_image_data_update_add(o, 0, 0, width, height);
   evas_object_resize(o, width, height);
   evas_object_show(o);

   target = o;

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, cb_move, ee);

   ecore_main_loop_begin();

   EINA_LIST_FREE(jobs, t)
     ecore_thread_cancel(t);

   ecore_evas_shutdown();

   return EXIT_SUCCESS;
}


