/**************************************************************************
 * An evas smart object template
 * 
 *
 ***************************************************************************/

#include <stdlib.h>
#include "X11_Trans.h"
#include "config.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <Imlib2.h>

static Evas_Smart * _x11_trans_object_smart_get(void);
/* smart object handlers */
static void _x11_trans_object_add(Evas_Object *o);
static void _x11_trans_object_del(Evas_Object *o);
static void _x11_trans_object_layer_set(Evas_Object *o, int l);
static void _x11_trans_object_raise(Evas_Object *o);
static void _x11_trans_object_lower(Evas_Object *o);
static void _x11_trans_object_stack_above(Evas_Object *o, Evas_Object *above);
static void _x11_trans_object_stack_below(Evas_Object *o, Evas_Object *below);
static void _x11_trans_object_move(Evas_Object *o, double x, double y);
static void _x11_trans_object_resize(Evas_Object *o, double w, double h);
static void _x11_trans_object_show(Evas_Object *o);
static void _x11_trans_object_hide(Evas_Object *o);
static void _x11_trans_object_color_set(Evas_Object *o, int r, int g, int b, int a);
static void _x11_trans_object_clip_set(Evas_Object *o, Evas_Object *clip);
static void _x11_trans_object_clip_unset(Evas_Object *o);

/* keep a global copy of this, so it only has to be created once */
void
evas_object_x11_trans_freshen(Evas_Object *o, int x, int y, int w, int h)
{
  Evas_Object_Trans *data;
  if((data = evas_object_smart_data_get(o)))
  {
      data->obj =
      transparency_get_pixmap(evas_object_evas_get(data->clip),
	data->obj, x, y, w, h);
      evas_object_pass_events_set(data->obj, 1);
      evas_object_clip_set(data->obj, data->clip);
      evas_object_move(data->clip, data->x, data->y);
      evas_object_resize(data->clip, data->w, data->h);
  }

}
/*** external API ***/

Evas_Object *
evas_object_x11_trans_new(Evas *e)
{
  Evas_Object_Trans *data;
  Evas_Object *x11_trans_object;
  
  x11_trans_object = evas_object_smart_add(e,
				_x11_trans_object_smart_get());
  return x11_trans_object;
}

/*** smart object handler functions ***/

static Evas_Smart *
_x11_trans_object_smart_get(void)
{
  Evas_Smart *smart = NULL;

  smart = evas_smart_new ("x11_trans_object",
                          _x11_trans_object_add,
                          _x11_trans_object_del,
                          _x11_trans_object_layer_set,
                          _x11_trans_object_raise,
                          _x11_trans_object_lower,
                          _x11_trans_object_stack_above,
                          _x11_trans_object_stack_below,
                          _x11_trans_object_move,
                          _x11_trans_object_resize,
                          _x11_trans_object_show,
                          _x11_trans_object_hide,
                          _x11_trans_object_color_set,
                          _x11_trans_object_clip_set,
                          _x11_trans_object_clip_unset,
                          NULL
                          );

  return smart; 
}

static void
_x11_trans_object_add(Evas_Object *o)
{
  Evas_Object_Trans *data;
 
  data = (Evas_Object_Trans*)malloc(sizeof(Evas_Object_Trans));
  memset(data, 0, sizeof(Evas_Object_Trans*));
      
  data->clip = evas_object_rectangle_add(evas_object_evas_get(o));
  evas_object_color_set(data->clip, 255, 255, 255, 255);
  evas_object_pass_events_set(data->clip, 1);
  
  evas_object_smart_data_set(o, data);
}


static void
_x11_trans_object_del(Evas_Object *o)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
  {
	if(data->obj)
	    evas_object_del(data->obj);
	if(data->clip)
	    evas_object_del(data->clip);
	data->obj = NULL;
	data->clip = NULL;
	free(data);
  }
}

static void
_x11_trans_object_layer_set(Evas_Object *o, int l)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
    evas_object_layer_set(data->clip, l);
}

static void
_x11_trans_object_raise(Evas_Object *o)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
    evas_object_raise(data->clip);
}

static void
_x11_trans_object_lower(Evas_Object *o)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
	evas_object_lower(data->clip);
}

static void
_x11_trans_object_stack_above(Evas_Object *o, Evas_Object *above)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
    evas_object_stack_above(data->clip, above);
}

static void
_x11_trans_object_stack_below(Evas_Object *o, Evas_Object *below)
{
  Evas_Object_Trans *data;
  
  data = evas_object_smart_data_get(o);

  if((data = evas_object_smart_data_get(o)))
    evas_object_stack_below(data->clip, below);
}

static void
_x11_trans_object_move(Evas_Object *o, double x, double y)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
  {
    if(data->x == x && data->y == y) return;
    evas_object_move(data->clip, x, y);
    data->x = x;
    data->y = y;
  }
}

static void
_x11_trans_object_resize(Evas_Object *o, double w, double h)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
  {
    if(data->w == w && data->h == h) return;
    evas_object_move(data->clip, w, h);
    data->w = w;
    data->h = h;
    evas_object_resize(data->clip, w, h);
  }
}

static void
_x11_trans_object_show(Evas_Object *o)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
    evas_object_show(data->clip);
}

static void
_x11_trans_object_hide(Evas_Object *o)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
    evas_object_hide(data->clip);
}

static void
_x11_trans_object_color_set(Evas_Object *o, int r, int g, int b, int a)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
  evas_object_color_set(data->clip, r, g, b, a);
}

static void
_x11_trans_object_clip_set(Evas_Object *o, Evas_Object *clip)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
    evas_object_clip_set(data->clip, clip);
}

static void
_x11_trans_object_clip_unset(Evas_Object *o)
{
  Evas_Object_Trans *data;
  
  if((data = evas_object_smart_data_get(o)))
    evas_object_clip_unset(data->clip);
}

/**
 * Stolen from ev and iconbar, hopefully this should be shareable now
 */
static Evas_Object *
transparency_get_pixmap(Evas *evas, Evas_Object *old,
                        int x, int y, int w, int h)
{
  Atom            prop,type; 
  int             format;
  unsigned long   length,after;
  unsigned char  *data;
  Evas_Object    *new=NULL;
  Pixmap          p;

  if(old)
    evas_object_del(old);

  if((prop=XInternAtom(ecore_x_display_get(),"_XROOTPMAP_ID",True))!=None)
  {
    int ret=XGetWindowProperty(ecore_x_display_get(), 
		               RootWindow(ecore_x_display_get(), 0),
                               prop, 0L, 1L, False, AnyPropertyType, &type,
                               &format,&length, &after,&data);
    if((ret==Success)&&(type==XA_PIXMAP)&&((p=*((Pixmap *)data))!=None)) {
      Imlib_Image  im;
      unsigned int pw,ph, pb,pd;
      int          px,py;
      Window       win_dummy;
      Status       st;

      st=XGetGeometry(ecore_x_display_get(),p,&win_dummy, &px,&py,&pw,&ph, &pb, &pd);
      if(st&&(pw>0)&&(ph>0)) {
#  ifdef NOIR_DEBUG
        fprintf(stderr,"bg_ebg_trans: transparency update %3d,%3d %3dx%3d\n",x,y,w,h);
#  endif

        imlib_context_set_display(ecore_x_display_get());
        imlib_context_set_visual(DefaultVisual(ecore_x_display_get(),DefaultScreen(ecore_x_display_get())));
        imlib_context_set_colormap(DefaultColormap(ecore_x_display_get(),DefaultScreen(ecore_x_display_get())));
        imlib_context_set_drawable(*((Pixmap *)data));

        im=imlib_create_image_from_drawable(0,x,y,w,h,1);
        imlib_context_set_image(im);
        imlib_image_set_format("argb");
        new=evas_object_image_add(evas);
        evas_object_image_alpha_set(new,0);
        evas_object_image_size_set(new,w,h);   /* thanks rephorm */
        evas_object_image_data_copy_set(new,imlib_image_get_data_for_reading_only());
        imlib_free_image();

        evas_object_image_fill_set(new,0,0,w,h);
        evas_object_resize(new,w,h);
        evas_object_move(new,0,0);
        evas_object_layer_set(new,-9999);
        evas_object_image_data_update_add(new,0,0,w,h);
        evas_object_show(new); }
#if 1
      else  /* this can happen with e16 */
        fprintf(stderr,"bg_ebg_trans: got invalid pixmap from root-window, ignoring...\n");
#endif
    }
    else
      fprintf(stderr,"bg_ebg_trans: could not read root-window property _XROOTPMAP_ID...\n"); }
  else
    fprintf(stderr,"bg_ebg_trans: could not get XAtom _XROOTPMAP_ID...\n");

  if(!new) {  /* fallback if no root pixmap is found */
#if 1
    fprintf(stderr,"bg_ebg_trans: cannot create transparency pixmap, no valid wallpaper set.\n");
#endif
    new=evas_object_rectangle_add(evas);
    evas_object_resize(new,w,h);
    evas_object_move(new,0,0);
    evas_object_layer_set(new,-9999);
    evas_object_color_set(new, 127,127,127, 255);
    evas_object_show(new); }

  return new; }
