#ifndef EFX_PRIVATE_H
#define EFX_PRIVATE_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <math.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include "Efx.h"

#ifndef __UNUSED__
# define __UNUSED__ __attribute__((unused))
#endif

#define DBG(...)            EINA_LOG_DOM_DBG(_efx_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(_efx_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(_efx_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(_efx_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(_efx_log_dom, __VA_ARGS__)

static const char *efx_speed_str[] =
{
   "LINEAR", "ACCELERATE", "DECELERATE", "SINUSOIDAL"
};

extern int _efx_log_dom;

typedef struct EFX
{
   Evas_Object *obj;
   void *spin_data;
   void *rotate_data;
   void *zoom_data;
   struct
     {
        double current;
        Evas_Point *center;
     } rotate;
   double current_zoom;
} EFX;

void _efx_zoom_calc(void *, Evas_Map *map);
void _efx_rotate_calc(void *, Evas_Map *map);
void _efx_spin_calc(void *, Evas_Map *map);


EFX *efx_new(Evas_Object *obj);
void efx_free(EFX *e);
Eina_Bool efx_rotate_center_init(EFX *e, const Evas_Point *center);

static inline void
_size_debug(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   DBG("%p: x=%d,y=%d,w=%d,h=%d", obj, x, y, w, h);
}

static inline void
efx_rotate_helper(EFX *e, Evas_Map *map, double degrees)
{
   if (e->rotate.center)
     evas_map_util_rotate(map, degrees, e->rotate.center->x, e->rotate.center->y);
   else
     {
        Evas_Coord x, y, w, h;
        evas_object_geometry_get(e->obj, &x, &y, &w, &h);
        evas_map_util_rotate(map, degrees, x + (w / 2), y + (h / 2));
     }
   //_size_debug(e->obj);
}

#endif
