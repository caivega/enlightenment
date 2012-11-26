#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_image.h"

#ifdef _WIN32
# define FMT_SIZE_T "%Iu"
#else
# define FMT_SIZE_T "%zu"
#endif

EAPI Eo_Op ELM_OBJ_IMAGE_BASE_ID = EO_NOOP;

#define MY_CLASS ELM_OBJ_IMAGE_CLASS
#define MY_CLASS_NAME "elm_image"

static const char SIG_DND[] = "drop";
static const char SIG_CLICKED[] = "clicked";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_DND, ""},
   {SIG_CLICKED, ""},
   {NULL, NULL}
};

static void
_on_image_preloaded(void *data,
                    Evas *e __UNUSED__,
                    Evas_Object *obj,
                    void *event __UNUSED__)
{
   Elm_Image_Smart_Data *sd = data;

   sd->preloading = EINA_FALSE;

   if (sd->show) evas_object_show(obj);
   if (sd->prev_img) evas_object_del(sd->prev_img);

   sd->prev_img = NULL;
}

static void
_on_mouse_up(void *data,
             Evas *e __UNUSED__,
             Evas_Object *obj __UNUSED__,
             void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
}

static Eina_Bool
_elm_image_animate_cb(void *data)
{
   Elm_Image_Smart_Data *sd = data;

   if (!sd->anim) return ECORE_CALLBACK_CANCEL;

   sd->cur_frame++;
   if (sd->cur_frame > sd->frame_count)
     sd->cur_frame = sd->cur_frame % sd->frame_count;

   evas_object_image_animated_frame_set(sd->img, sd->cur_frame);

   sd->frame_duration = evas_object_image_animated_frame_duration_get
       (sd->img, sd->cur_frame, 0);

   if (sd->frame_duration > 0)
     ecore_timer_interval_set(sd->anim_timer, sd->frame_duration);

   return ECORE_CALLBACK_RENEW;
}

static Evas_Object *
_img_new(Evas_Object *obj)
{
   Evas_Object *img;

   ELM_IMAGE_DATA_GET(obj, sd);

   img = evas_object_image_add(evas_object_evas_get(obj));
   evas_object_image_scale_hint_set(img, EVAS_IMAGE_SCALE_HINT_STATIC);
   evas_object_repeat_events_set(img, EINA_TRUE);
   evas_object_event_callback_add
     (img, EVAS_CALLBACK_IMAGE_PRELOADED, _on_image_preloaded, sd);
   evas_object_event_callback_add
     (img, EVAS_CALLBACK_MOUSE_UP, _on_mouse_up, obj);

   evas_object_smart_member_add(img, obj);
   elm_widget_sub_object_add(obj, img);

   return img;
}

static void
_elm_image_internal_sizing_eval(Evas_Object *obj, Elm_Image_Smart_Data *sd)
{
   Evas_Coord x, y, w, h;
   const char *type;

   if (!sd->img) return;

   w = sd->img_w;
   h = sd->img_h;

   type = evas_object_type_get(sd->img);
   if (!type) return;

   if (!strcmp(type, "edje"))
     {
        x = sd->img_x;
        y = sd->img_y;
        evas_object_move(sd->img, x, y);
        evas_object_resize(sd->img, w, h);
     }
   else
     {
        double alignh = 0.5, alignv = 0.5;
        int iw = 0, ih = 0;

        evas_object_image_size_get(sd->img, &iw, &ih);

        iw = ((double)iw) * sd->scale;
        ih = ((double)ih) * sd->scale;

        if (iw < 1) iw = 1;
        if (ih < 1) ih = 1;

        if (sd->aspect_fixed)
          {
             h = ((double)ih * w) / (double)iw;
             if (sd->fill_inside)
               {
                  if (h > sd->img_h)
                    {
                       h = sd->img_h;
                       w = ((double)iw * h) / (double)ih;
                    }
               }
             else
               {
                  if (h < sd->img_h)
                    {
                       h = sd->img_h;
                       w = ((double)iw * h) / (double)ih;
                    }
               }
          }
        if (!sd->resize_up)
          {
             if (w > iw) w = iw;
             if (h > ih) h = ih;
          }
        if (!sd->resize_down)
          {
             if (w < iw) w = iw;
             if (h < ih) h = ih;
          }

        evas_object_size_hint_align_get
           (obj, &alignh, &alignv);

        if (alignh == EVAS_HINT_FILL) alignh = 0.5;
        if (alignv == EVAS_HINT_FILL) alignv = 0.5;

        x = sd->img_x + ((sd->img_w - w) * alignh);
        y = sd->img_y + ((sd->img_h - h) * alignv);

        evas_object_move(sd->img, x, y);
        evas_object_image_fill_set(sd->img, 0, 0, w, h);
        evas_object_resize(sd->img, w, h);
     }
}

static void
_elm_image_file_set_do(Evas_Object *obj)
{
   Evas_Object *pclip = NULL;

   ELM_IMAGE_DATA_GET(obj, sd);

   if (sd->prev_img) evas_object_del(sd->prev_img);
   if (sd->img)
     {
        pclip = evas_object_clip_get(sd->img);
        sd->prev_img = sd->img;
     }

   sd->img = _img_new(obj);

   evas_object_image_load_orientation_set(sd->img, EINA_TRUE);

   evas_object_clip_set(sd->img, pclip);

   sd->edje = EINA_FALSE;

   if (!sd->load_size)
     evas_object_image_load_size_set(sd->img, sd->load_size, sd->load_size);
}

/* WARNING: whenever you patch this function, remember to do the same
 * on elm_icon.c:_elm_icon_smart_file_set()'s 2nd half.
 */
static Eina_Bool
_elm_image_edje_file_set(Evas_Object *obj,
                         const char *file,
                         const char *group)
{
   Evas_Object *pclip;

   ELM_IMAGE_DATA_GET(obj, sd);

   if (sd->prev_img) evas_object_del(sd->prev_img);
   sd->prev_img = NULL;

   if (!sd->edje)
     {
        pclip = evas_object_clip_get(sd->img);
        if (sd->img) evas_object_del(sd->img);

        /* Edje object instead */
        sd->img = edje_object_add(evas_object_evas_get(obj));
        evas_object_smart_member_add(sd->img, obj);
        if (sd->show) evas_object_show(sd->img);
        evas_object_clip_set(sd->img, pclip);
        evas_object_event_callback_add
           (sd->img, EVAS_CALLBACK_MOUSE_UP, _on_mouse_up, obj);
     }

   sd->edje = EINA_TRUE;
   if (!edje_object_file_set(sd->img, file, group))
     {
        ERR("failed to set edje file '%s', group '%s': %s", file, group,
            edje_load_error_str(edje_object_load_error_get(sd->img)));
        return EINA_FALSE;
     }

   /* FIXME: do i want to update icon on file change ? */
   _elm_image_internal_sizing_eval(obj, sd);

   return EINA_TRUE;
}

static void
_elm_image_smart_size_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   int *w = va_arg(*list, int *);
   int *h = va_arg(*list, int *);

   int tw, th;
   int cw = 0, ch = 0;
   const char *type;

   Elm_Image_Smart_Data *sd = _pd;

   type = evas_object_type_get(sd->img);
   if (!type) return;

   if (!strcmp(type, "edje"))
     edje_object_size_min_get(sd->img, &tw, &th);
   else
     evas_object_image_size_get(sd->img, &tw, &th);

   if ((sd->resize_up) || (sd->resize_down))
     evas_object_geometry_get(sd->img, NULL, NULL, &cw, &ch);

   tw = tw > cw ? tw : cw;
   th = th > ch ? th : ch;
   tw = ((double)tw) * sd->scale;
   th = ((double)th) * sd->scale;
   if (w) *w = tw;
   if (h) *h = th;
}

static void
_elm_image_smart_smooth_scale_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool smooth = va_arg(*list, int);

   Elm_Image_Smart_Data *sd = _pd;

   if (sd->edje) return;

   evas_object_image_smooth_scale_set(sd->img, smooth);
}

static void
_elm_image_smart_smooth_scale_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   Elm_Image_Smart_Data *sd = _pd;

   if (sd->edje)
     {
        *ret = EINA_FALSE;
        return;
     }

   *ret = evas_object_image_smooth_scale_get(sd->img);
}

static void
_elm_image_smart_object_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Evas_Object **ret = va_arg(*list, Evas_Object **);

   Elm_Image_Smart_Data *sd = _pd;

   *ret = sd->img;
}

static void
_elm_image_smart_fill_inside_set(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool fill_inside = va_arg(*list, int);

   Elm_Image_Smart_Data *sd = _pd;

   fill_inside = !!fill_inside;

   if (sd->fill_inside == fill_inside) return;

   sd->fill_inside = fill_inside;

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_fill_inside_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   Elm_Image_Smart_Data *sd = _pd;

   *ret =  sd->fill_inside;
}

static void
_elm_image_smart_resize_up_set(Eo *obj, void *_pd, va_list *list)

{
   Eina_Bool resize_up = va_arg(*list, int);

   Elm_Image_Smart_Data *sd = _pd;

   resize_up = !!resize_up;

   if (sd->resize_up == resize_up) return;

   sd->resize_up = resize_up;

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_resize_up_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   Elm_Image_Smart_Data *sd = _pd;

   *ret = sd->resize_up;
}

static void
_elm_image_smart_resize_down_set(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool resize_down = va_arg(*list, int);

   Elm_Image_Smart_Data *sd = _pd;

   resize_down = !!resize_down;

   if (sd->resize_down == resize_down) return;

   sd->resize_down = resize_down;

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_resize_down_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   Elm_Image_Smart_Data *sd = _pd;
   *ret = sd->resize_up;
}

static void
_elm_image_flip_horizontal(Evas_Object *obj, Elm_Image_Smart_Data *sd)
{
   unsigned int *p1, *p2, tmp;
   unsigned int *data;
   int x, y, iw, ih;

   evas_object_image_size_get(sd->img, &iw, &ih);
   data = evas_object_image_data_get(sd->img, EINA_TRUE);

   for (y = 0; y < ih; y++)
     {
        p1 = data + (y * iw);
        p2 = data + ((y + 1) * iw) - 1;
        for (x = 0; x < (iw >> 1); x++)
          {
             tmp = *p1;
             *p1 = *p2;
             *p2 = tmp;
             p1++;
             p2--;
          }
     }

   evas_object_image_data_set(sd->img, data);
   evas_object_image_data_update_add(sd->img, 0, 0, iw, ih);

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_flip_vertical(Evas_Object *obj, Elm_Image_Smart_Data *sd)
{
   unsigned int *p1, *p2, tmp;
   unsigned int *data;
   int x, y, iw, ih;

   evas_object_image_size_get(sd->img, &iw, &ih);
   data = evas_object_image_data_get(sd->img, EINA_TRUE);

   for (y = 0; y < (ih >> 1); y++)
     {
        p1 = data + (y * iw);
        p2 = data + ((ih - 1 - y) * iw);
        for (x = 0; x < iw; x++)
          {
             tmp = *p1;
             *p1 = *p2;
             *p2 = tmp;
             p1++;
             p2++;
          }
     }

   evas_object_image_data_set(sd->img, data);
   evas_object_image_data_update_add(sd->img, 0, 0, iw, ih);

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_rotate_180(Evas_Object *obj, Elm_Image_Smart_Data *sd)
{
   unsigned int *p1, *p2, tmp;
   unsigned int *data;
   int x, hw, iw, ih;

   evas_object_image_size_get(sd->img, &iw, &ih);
   data = evas_object_image_data_get(sd->img, 1);

   hw = iw * ih;
   x = (hw / 2);
   p1 = data;
   p2 = data + hw - 1;

   for (; --x > 0; )
     {
        tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
     }

   evas_object_image_data_set(sd->img, data);
   evas_object_image_data_update_add(sd->img, 0, 0, iw, ih);

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_orient_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Image_Orient orient = va_arg(*list, Elm_Image_Orient);

   unsigned int *data, *data2 = NULL, *to, *from;
   int x, y, w, hw, iw, ih;

   Elm_Image_Smart_Data *sd = _pd;

   if (sd->edje)
     return;

   switch (orient)
     {
      case ELM_IMAGE_FLIP_HORIZONTAL:
        _elm_image_flip_horizontal(obj, sd);
	sd->orient = orient;
        return;

      case ELM_IMAGE_FLIP_VERTICAL:
        _elm_image_flip_vertical(obj, sd);
	sd->orient = orient;
        return;

      case ELM_IMAGE_ROTATE_180:
        _elm_image_smart_rotate_180(obj, sd);
	sd->orient = orient;
        return;

     case ELM_IMAGE_ORIENT_NONE:
        sd->orient = orient;
        return;

      default:
        break;
     }

   evas_object_image_size_get(sd->img, &iw, &ih);

   /* we need separate destination memory if we want to rotate 90 or
    * 270 degree */
   data = evas_object_image_data_get(sd->img, EINA_FALSE);
   if (!data) return;
   data2 = malloc(sizeof(unsigned char) * (iw * ih * 4));
   if (!data2) return;
   memcpy(data2, data, sizeof (unsigned char) * (iw * ih * 4));

   w = ih;
   ih = iw;
   iw = w;
   hw = w * ih;

   evas_object_image_size_set(sd->img, iw, ih);
   data = evas_object_image_data_get(sd->img, EINA_TRUE);

   switch (orient)
     {
      case ELM_IMAGE_FLIP_TRANSPOSE:
        to = data;
        hw = -hw + 1;
	sd->orient = orient;
        break;

      case ELM_IMAGE_FLIP_TRANSVERSE:
        to = data + hw - 1;
        w = -w;
        hw = hw - 1;
	sd->orient = orient;
        break;

      case ELM_IMAGE_ROTATE_90:
        to = data + w - 1;
        hw = -hw - 1;
	sd->orient = orient;
        break;

      case ELM_IMAGE_ROTATE_270:
        to = data + hw - w;
        w = -w;
        hw = hw + 1;
	sd->orient = orient;
        break;

      default:
        ERR("unknown orient %d", orient);
        evas_object_image_data_set(sd->img, data);  // give it back
        if (data2) free(data2);

        return;
     }

   from = data2;
   for (x = iw; --x >= 0; )
     {
        for (y = ih; --y >= 0; )
          {
             *to = *from;
             from++;
             to += w;
          }
        to += hw;
     }
   if (data2) free(data2);

   evas_object_image_data_set(sd->img, data);
   evas_object_image_data_update_add(sd->img, 0, 0, iw, ih);

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_orient_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   Elm_Image_Orient *ret = va_arg(*list, Elm_Image_Orient *);

   *ret = sd->orient;
}

static Eina_Bool
_elm_image_drag_n_drop_cb(void *elm_obj,
                          Evas_Object *obj,
                          Elm_Selection_Data *drop)
{
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_image_file_set(drop->data, NULL, &ret));
   if(ret)
     {
        printf("dnd: %s, %s, %s", elm_widget_type_get(elm_obj),
               SIG_DND, (char *)drop->data);

        evas_object_smart_callback_call(elm_obj, SIG_DND, drop->data);
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

static void
_elm_image_smart_add(Eo *obj, void *_pd, va_list *list EINA_UNUSED)
{
   eo_do_super(obj, evas_obj_smart_add());

   Elm_Image_Smart_Data *priv = _pd;

   /* starts as an Evas image. may switch to an Edje object */
   priv->img = _img_new(obj);
   priv->prev_img = NULL;

   priv->smooth = EINA_TRUE;
   priv->fill_inside = EINA_TRUE;
   priv->resize_up = EINA_TRUE;
   priv->resize_down = EINA_TRUE;
   priv->aspect_fixed = EINA_TRUE;

   priv->load_size = 64;
   priv->scale = 1.0;

   eo_do(obj, elm_obj_image_load_size_set(0));

   elm_widget_can_focus_set(obj, EINA_FALSE);

   eo_do(obj, elm_obj_image_sizing_eval());
}

static void
_elm_image_smart_del(Eo *obj, void *_pd, va_list *list EINA_UNUSED)
{
   Elm_Image_Smart_Data *sd = _pd;

   if (sd->anim_timer)
     ecore_timer_del(sd->anim_timer);

   evas_object_del(sd->img);
   if (sd->prev_img) evas_object_del(sd->prev_img);

   eo_do_super(obj, evas_obj_smart_del());
}

static void
_elm_image_smart_move(Eo *obj, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   Evas_Coord x = va_arg(*list, Evas_Coord);
   Evas_Coord y = va_arg(*list, Evas_Coord);

   eo_do_super(obj, evas_obj_smart_move(x, y));

   if ((sd->img_x == x) && (sd->img_y == y)) return;
   sd->img_x = x;
   sd->img_y = y;

   /* takes care of moving */
   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_resize(Eo *obj, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;
   Evas_Coord w = va_arg(*list, Evas_Coord);
   Evas_Coord h = va_arg(*list, Evas_Coord);

   eo_do_super(obj, evas_obj_smart_resize(w, h));

   if ((sd->img_w == w) && (sd->img_h == h)) return;

   sd->img_w = w;
   sd->img_h = h;

   /* takes care of resizing */
   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_show(Eo *obj, void *_pd, va_list *list EINA_UNUSED)
{
   Elm_Image_Smart_Data *sd = _pd;

   sd->show = EINA_TRUE;
   if (sd->preloading)
     return;

   eo_do_super(obj, evas_obj_smart_show());

   evas_object_show(sd->img);

   if (sd->prev_img) evas_object_del(sd->prev_img);
   sd->prev_img = NULL;
}

static void
_elm_image_smart_hide(Eo *obj, void *_pd, va_list *list EINA_UNUSED)
{
   Elm_Image_Smart_Data *sd = _pd;

   eo_do_super(obj, evas_obj_smart_hide());

   sd->show = EINA_FALSE;
   evas_object_hide(sd->img);

   if (sd->prev_img) evas_object_del(sd->prev_img);
   sd->prev_img = NULL;
}

static void
_elm_image_smart_color_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   int r = va_arg(*list, int);
   int g = va_arg(*list, int);
   int b = va_arg(*list, int);
   int a = va_arg(*list, int);

   eo_do_super(obj, evas_obj_smart_color_set(r, g, b, a));

   evas_object_color_set(sd->img, r, g, b, a);
   if (sd->prev_img) evas_object_color_set(sd->prev_img, r, g, b, a);
}

static void
_elm_image_smart_clip_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   Evas_Object *clip = va_arg(*list, Evas_Object *);

   eo_do_super(obj, evas_obj_smart_clip_set(clip));

   evas_object_clip_set(sd->img, clip);
   if (sd->prev_img) evas_object_clip_set(sd->prev_img, clip);
}

static void
_elm_image_smart_clip_unset(Eo *obj, void *_pd, va_list *list EINA_UNUSED)

{
   Elm_Image_Smart_Data *sd = _pd;

   eo_do_super(obj, evas_obj_smart_clip_unset());

   evas_object_clip_unset(sd->img);
   if (sd->prev_img) evas_object_clip_unset(sd->prev_img);
}

static void
_elm_image_smart_theme(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
   Eina_Bool int_ret = EINA_FALSE;

   eo_do_super(obj, elm_wdg_theme(&int_ret));
   if (!int_ret) return;

   eo_do(obj, elm_obj_image_sizing_eval());

   if (ret) *ret = EINA_TRUE;
}

static void
_elm_image_smart_sizing_eval(Eo *obj, void *_pd, va_list *list EINA_UNUSED)
{
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   int w, h;
   double ts;

   Elm_Image_Smart_Data *sd = _pd;

   _elm_image_internal_sizing_eval(obj, sd);

   eo_do(obj, elm_obj_image_smooth_scale_set(sd->smooth));

   if (sd->no_scale)
     eo_do(obj, elm_obj_image_scale_set(1.0));
   else
     eo_do(obj, elm_obj_image_smooth_scale_set(elm_widget_scale_get(obj) * elm_config_scale_get()));

   ts = sd->scale;
   sd->scale = 1.0;
   eo_do(obj, elm_obj_image_size_get(&w, &h));

   sd->scale = ts;
   evas_object_size_hint_min_get(obj, &minw, &minh);

   if (sd->no_scale)
     {
        maxw = minw = w;
        maxh = minh = h;
        if ((sd->scale > 1.0) && (sd->resize_up))
          {
             maxw = minw = w * sd->scale;
             maxh = minh = h * sd->scale;
          }
        else if ((sd->scale < 1.0) && (sd->resize_down))
          {
             maxw = minw = w * sd->scale;
             maxh = minh = h * sd->scale;
          }
     }
   else
     {
        if (!sd->resize_down)
          {
             minw = w * sd->scale;
             minh = h * sd->scale;
          }
        if (!sd->resize_up)
          {
             maxw = w * sd->scale;
             maxh = h * sd->scale;
          }
     }

   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_elm_image_smart_memfile_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   const void *img = va_arg(*list, const void *);
   size_t size = va_arg(*list, size_t);
   const char *format = va_arg(*list, const char *);
   const char *key = va_arg(*list, const char *);
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   _elm_image_file_set_do(obj);

   evas_object_image_memfile_set
     (sd->img, (void *)img, size, (char *)format, (char *)key);

   sd->preloading = EINA_TRUE;
   sd->show = EINA_TRUE;

   evas_object_hide(sd->img);
   evas_object_image_preload(sd->img, EINA_FALSE);
   if (evas_object_image_load_error_get(sd->img) != EVAS_LOAD_ERROR_NONE)
     {
        ERR("Things are going bad for some random " FMT_SIZE_T
            " byte chunk of memory (%p)", size, sd->img);
        if (ret) *ret = EINA_FALSE;
        return;
     }

   _elm_image_internal_sizing_eval(obj, sd);

   if (ret) *ret = EINA_TRUE;
}

static void
_elm_image_smart_file_set(Eo *obj, void *_pd, va_list *list)
{
   const char *file = va_arg(*list, const char *);
   const char *key = va_arg(*list, const char *);
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   Evas_Coord w, h;

   Elm_Image_Smart_Data *sd = _pd;

   if (eina_str_has_extension(file, ".edj"))
     {
        Eina_Bool int_ret = _elm_image_edje_file_set(obj, file, key);
        if (ret) *ret = int_ret;
        return;
     }

   _elm_image_file_set_do(obj);

   evas_object_image_file_set(sd->img, file, key);

   sd->preloading = EINA_TRUE;
   sd->show = EINA_TRUE;

   evas_object_hide(sd->img);

   eo_do(obj, elm_obj_image_size_get(&w, &h));

   evas_object_image_load_size_set(sd->img, w, h);

   evas_object_image_preload(sd->img, EINA_FALSE);
   if (evas_object_image_load_error_get(sd->img) != EVAS_LOAD_ERROR_NONE)
     {
        ERR("Things are going bad for '%s' (%p)", file, sd->img);
        if (ret) *ret = EINA_FALSE;
        return;
     }

   _elm_image_internal_sizing_eval(obj, sd);

   if (ret) *ret = EINA_TRUE;
}

static void
_elm_image_smart_file_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   const char **file = va_arg(*list, const char **);
   const char **key = va_arg(*list, const char **);

   if (sd->edje)
     edje_object_file_get(sd->img, file, key);
   else
     evas_object_image_file_get(sd->img, file, key);
}

static void
_elm_image_smart_preload_disabled_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   Eina_Bool disable = va_arg(*list, int);

   if (sd->edje) return;

   evas_object_image_preload(sd->img, disable);
   sd->preloading = !disable;
}

static void
_elm_image_smart_load_size_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   int size = va_arg(*list, int);

   sd->load_size = size;
   if (!sd->img || sd->edje) return;

   evas_object_image_load_size_set(sd->img, sd->load_size, sd->load_size);
}

static void
_elm_image_smart_load_size_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   int *ret = va_arg(*list, int *);

   *ret = sd->load_size;
}

static void
_elm_image_smart_scale_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;

   double scale = va_arg(*list, double);

   sd->scale = scale;

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_scale_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;
   double *ret = va_arg(*list, double *);

   *ret = sd->scale;
}

/**
 * Turns on editing through drag and drop and copy and paste.
 */
static void
_elm_image_smart_editable_set(Eo *obj, void *_pd, va_list *list)

{
   Elm_Image_Smart_Data *sd = _pd;
   Eina_Bool edit = va_arg(*list, int);
   Evas_Object *parent = va_arg(*list, Evas_Object *);

   if (sd->edje)
     {
        printf("No editing edje objects yet (ever)\n");
        return;
     }

   edit = !!edit;

   if (edit == sd->edit) return;

   sd->edit = edit;

   if (sd->edit)
     elm_drop_target_add
       (obj, ELM_SEL_FORMAT_IMAGE, _elm_image_drag_n_drop_cb, parent);
   else
     elm_drop_target_del(obj);
}

static void
_elm_image_smart_editable_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   *ret = sd->edit;
}

static void
_elm_image_smart_aspect_fixed_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;
   Eina_Bool fixed = va_arg(*list, int);

   fixed = !!fixed;
   if (sd->aspect_fixed == fixed) return;

   sd->aspect_fixed = fixed;

   _elm_image_internal_sizing_eval(obj, sd);
}

static void
_elm_image_smart_aspect_fixed_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Image_Smart_Data *sd = _pd;
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   *ret = sd->aspect_fixed;
}

EAPI Evas_Object *
elm_image_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);
   eo_unref(obj);
   return obj;
}

static void
_constructor(Eo *obj, void *_pd EINA_UNUSED, va_list *list EINA_UNUSED)
{
   eo_do_super(obj, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME),
         evas_obj_smart_callbacks_descriptions_set(_smart_callbacks, NULL));

   if (!elm_widget_sub_object_add(eo_parent_get(obj), obj))
     ERR("could not add %p as sub object of %p", obj, eo_parent_get(obj));
}

EAPI Eina_Bool
elm_image_memfile_set(Evas_Object *obj,
                      const void *img,
                      size_t size,
                      const char *format,
                      const char *key)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(img, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!size, EINA_FALSE);

   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_image_memfile_set(img, size, format, key, &ret));
   return ret;
}

EAPI Eina_Bool
elm_image_file_set(Evas_Object *obj,
                   const char *file,
                   const char *group)
{
   Eina_Bool ret = EINA_FALSE;

   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   EINA_SAFETY_ON_NULL_RETURN_VAL(file, EINA_FALSE);
   eo_do(obj, elm_obj_image_file_set(file, group, &ret));
   eo_do(obj, elm_obj_image_sizing_eval());
   return ret;
}

EAPI void
elm_image_file_get(const Evas_Object *obj,
                   const char **file,
                   const char **group)
{
   ELM_IMAGE_CHECK(obj);
   eo_do((Eo *) obj, elm_obj_image_file_get(file, group));
}

EAPI void
elm_image_smooth_set(Evas_Object *obj,
                     Eina_Bool smooth)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_smooth_set(smooth));
}

static void
_elm_image_smart_smooth_set(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool smooth = va_arg(*list, int);
   Elm_Image_Smart_Data *sd = _pd;
   sd->smooth = smooth;

   eo_do(obj, elm_obj_image_sizing_eval());
}

EAPI Eina_Bool
elm_image_smooth_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_image_smooth_get(&ret));
   return ret;
}

static void
_elm_image_smart_smooth_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   Elm_Image_Smart_Data *sd = _pd;
   *ret =  sd->smooth;
}

EAPI void
elm_image_object_size_get(const Evas_Object *obj,
                          int *w,
                          int *h)
{
   if (w) *w = 0;
   if (h) *h = 0;

   ELM_IMAGE_CHECK(obj);
   eo_do((Eo *) obj, elm_obj_image_size_get(w, h));
}

EAPI void
elm_image_no_scale_set(Evas_Object *obj,
                       Eina_Bool no_scale)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_no_scale_set(no_scale));
}

static void
_elm_image_smart_no_scale_set(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool no_scale = va_arg(*list, int);
   Elm_Image_Smart_Data *sd = _pd;
   sd->no_scale = no_scale;

   eo_do(obj, elm_obj_image_sizing_eval());
}

EAPI Eina_Bool
elm_image_no_scale_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_image_no_scale_get(&ret));
   return ret;
}

static void
_elm_image_smart_no_scale_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   Elm_Image_Smart_Data *sd = _pd;
   *ret = sd->no_scale;
}

EAPI void
elm_image_resizable_set(Evas_Object *obj,
                        Eina_Bool up,
                        Eina_Bool down)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_resizable_set(up, down));
}

static void
_elm_image_smart_resizable_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool up = va_arg(*list, int);
   Eina_Bool down = va_arg(*list, int);
   Elm_Image_Smart_Data *sd = _pd;

   sd->resize_up = !!up;
   sd->resize_down = !!down;

   eo_do(obj, elm_obj_image_sizing_eval());
}

EAPI void
elm_image_resizable_get(const Evas_Object *obj,
                        Eina_Bool *size_up,
                        Eina_Bool *size_down)
{
   ELM_IMAGE_CHECK(obj);
   eo_do((Eo *) obj, elm_obj_image_resizable_get(size_up, size_down));
}

static void
_elm_image_smart_resizable_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *size_up = va_arg(*list, Eina_Bool *);
   Eina_Bool *size_down = va_arg(*list, Eina_Bool *);

   Elm_Image_Smart_Data *sd = _pd;
   if (size_up) *size_up = sd->resize_up;
   if (size_down) *size_down = sd->resize_down;
}

EAPI void
elm_image_fill_outside_set(Evas_Object *obj,
                           Eina_Bool fill_outside)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_fill_outside_set(fill_outside));
}

static void
_elm_image_smart_fill_outside_set(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool fill_outside = va_arg(*list, int);
   Elm_Image_Smart_Data *sd = _pd;
   sd->fill_inside = !fill_outside;

   eo_do(obj, elm_obj_image_sizing_eval());
}


EAPI Eina_Bool
elm_image_fill_outside_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_image_fill_outside_get(&ret));
   return ret;
}

static void
_elm_image_smart_fill_outside_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   Elm_Image_Smart_Data *sd = _pd;
   *ret = !sd->fill_inside;
}

EAPI void
elm_image_preload_disabled_set(Evas_Object *obj,
                               Eina_Bool disabled)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_preload_disabled_set(!!disabled));
}

EAPI void
elm_image_prescale_set(Evas_Object *obj,
                       int size)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_load_size_set(size));
}

EAPI int
elm_image_prescale_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) 0;

   int ret = 0;
   eo_do((Eo *)obj, elm_obj_image_load_size_get(&ret));
   return ret;
}

EAPI void
elm_image_orient_set(Evas_Object *obj,
                     Elm_Image_Orient orient)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_orient_set(orient));
}

EAPI Elm_Image_Orient
elm_image_orient_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) ELM_IMAGE_ORIENT_NONE;

   Elm_Image_Orient ret = ELM_IMAGE_ORIENT_NONE;
   eo_do((Eo *) obj, elm_obj_image_orient_get(&ret));

   return ret;
}

EAPI void
elm_image_editable_set(Evas_Object *obj,
                       Eina_Bool set)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_editable_set(set, obj));
}

EAPI Eina_Bool
elm_image_editable_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_image_editable_get(&ret));
   return ret;
}

EAPI Evas_Object *
elm_image_object_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) NULL;

   Evas_Object *ret = NULL;
   eo_do((Eo *) obj, elm_obj_image_object_get(&ret));
   return ret;
}

EAPI void
elm_image_aspect_fixed_set(Evas_Object *obj,
                           Eina_Bool fixed)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_aspect_fixed_set(fixed));
}

EAPI Eina_Bool
elm_image_aspect_fixed_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_image_aspect_fixed_get(&ret));
   return ret;
}

EAPI Eina_Bool
elm_image_animated_available_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_image_animated_available_get(&ret));
   return ret;
}

static void
_elm_image_smart_animated_available_get(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);

   Elm_Image_Smart_Data *sd = _pd;

   if (sd->edje)
     {
        *ret = EINA_FALSE;
        return;
     }

   *ret = evas_object_image_animated_get(elm_image_object_get(obj));
}

EAPI void
elm_image_animated_set(Evas_Object *obj,
                       Eina_Bool anim)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_animated_set(anim));
}

static void
_elm_image_smart_animated_set(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool anim = va_arg(*list, int);
   Elm_Image_Smart_Data *sd = _pd;

   anim = !!anim;
   if (sd->anim == anim) return;

   if (sd->edje) return;

   sd->img = elm_image_object_get(obj);
   if (!evas_object_image_animated_get(sd->img)) return;

   if (anim)
     {
        sd->frame_count = evas_object_image_animated_frame_count_get(sd->img);
        sd->cur_frame = 1;
        sd->frame_duration =
          evas_object_image_animated_frame_duration_get
            (sd->img, sd->cur_frame, 0);
        evas_object_image_animated_frame_set(sd->img, sd->cur_frame);
     }
   else
     {
        sd->frame_count = -1;
        sd->cur_frame = -1;
        sd->frame_duration = -1;
     }
   sd->anim = anim;

   return;
}

EAPI Eina_Bool
elm_image_animated_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_image_animated_get(&ret));
   return ret;
}

static void
_elm_image_smart_animated_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   Elm_Image_Smart_Data *sd = _pd;
   *ret =  sd->anim;
}

EAPI void
elm_image_animated_play_set(Evas_Object *obj,
                            Eina_Bool play)
{
   ELM_IMAGE_CHECK(obj);
   eo_do(obj, elm_obj_image_animated_play_set(play));
}

static void
_elm_image_smart_animated_play_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool play = va_arg(*list, int);
   Elm_Image_Smart_Data *sd = _pd;

   if (!sd->anim) return;
   if (sd->play == play) return;

   if (sd->edje) return;

   if (play)
     {
        sd->anim_timer = ecore_timer_add
            (sd->frame_duration, _elm_image_animate_cb, sd);
     }
   else
     {
        if (sd->anim_timer)
          {
             ecore_timer_del(sd->anim_timer);
             sd->anim_timer = NULL;
          }
     }
   sd->play = play;
}

EAPI Eina_Bool
elm_image_animated_play_get(const Evas_Object *obj)
{
   ELM_IMAGE_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_image_animated_play_get(&ret));
   return ret;
}

static void
_elm_image_smart_animated_play_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   Elm_Image_Smart_Data *sd = _pd;
   *ret = sd->play;
}

static void
_class_constructor(Eo_Class *klass)
{
   const Eo_Op_Func_Description func_desc[] = {
        EO_OP_FUNC(EO_BASE_ID(EO_BASE_SUB_ID_CONSTRUCTOR), _constructor),

        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_ADD), _elm_image_smart_add),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_DEL), _elm_image_smart_del),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_RESIZE), _elm_image_smart_resize),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_MOVE), _elm_image_smart_move),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_SHOW), _elm_image_smart_show),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_HIDE), _elm_image_smart_hide),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_COLOR_SET), _elm_image_smart_color_set),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_CLIP_SET), _elm_image_smart_clip_set),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_CLIP_UNSET), _elm_image_smart_clip_unset),

        EO_OP_FUNC(ELM_WIDGET_ID(ELM_WIDGET_SUB_ID_THEME), _elm_image_smart_theme),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ASPECT_FIXED_SET), _elm_image_smart_aspect_fixed_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ASPECT_FIXED_GET), _elm_image_smart_aspect_fixed_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_EDITABLE_SET), _elm_image_smart_editable_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_EDITABLE_GET), _elm_image_smart_editable_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_FILE_SET), _elm_image_smart_file_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_FILE_GET), _elm_image_smart_file_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_SIZING_EVAL), _elm_image_smart_sizing_eval),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_SMOOTH_SCALE_SET), _elm_image_smart_smooth_scale_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_SMOOTH_SCALE_GET), _elm_image_smart_smooth_scale_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_FILL_INSIDE_SET), _elm_image_smart_fill_inside_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_FILL_INSIDE_GET), _elm_image_smart_fill_inside_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_FILL_OUTSIDE_SET), _elm_image_smart_fill_outside_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_FILL_OUTSIDE_GET), _elm_image_smart_fill_outside_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_OBJECT_GET), _elm_image_smart_object_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_LOAD_SIZE_SET), _elm_image_smart_load_size_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_LOAD_SIZE_GET), _elm_image_smart_load_size_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_MEMFILE_SET), _elm_image_smart_memfile_set),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ORIENT_SET), _elm_image_smart_orient_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ORIENT_GET), _elm_image_smart_orient_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_PRELOAD_DISABLED_SET), _elm_image_smart_preload_disabled_set),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_RESIZE_DOWN_SET), _elm_image_smart_resize_down_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_RESIZE_DOWN_GET), _elm_image_smart_resize_down_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_RESIZE_UP_SET), _elm_image_smart_resize_up_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_RESIZE_UP_GET), _elm_image_smart_resize_up_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_SCALE_SET), _elm_image_smart_scale_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_SCALE_GET), _elm_image_smart_scale_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_OBJECT_SIZE_GET), _elm_image_smart_size_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_SMOOTH_SET), _elm_image_smart_smooth_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_SMOOTH_GET), _elm_image_smart_smooth_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_NO_SCALE_SET), _elm_image_smart_no_scale_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_NO_SCALE_GET), _elm_image_smart_no_scale_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_RESIZABLE_SET), _elm_image_smart_resizable_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_RESIZABLE_GET), _elm_image_smart_resizable_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_AVAILABLE_GET), _elm_image_smart_animated_available_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_SET), _elm_image_smart_animated_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_GET), _elm_image_smart_animated_get),

        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_PLAY_SET), _elm_image_smart_animated_play_set),
        EO_OP_FUNC(ELM_OBJ_IMAGE_ID(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_PLAY_GET), _elm_image_smart_animated_play_get),

        EO_OP_FUNC_SENTINEL
   };
   eo_class_funcs_set(klass, func_desc);
}

static const Eo_Op_Description op_desc[] = {
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ASPECT_FIXED_SET, "Set whether the original aspect ratio of the image should be kept on resize."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ASPECT_FIXED_GET, "Get if the object retains the original aspect ratio."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_EDITABLE_SET, "Make the image 'editable'."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_EDITABLE_GET, "Check if the image is 'editable'."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_FILE_SET, "Set the file that will be used as the image's source."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_FILE_GET, "Get the file that will be used as image."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_SIZING_EVAL, "'Virtual' function on evalutating the object's final geometry."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_SMOOTH_SCALE_SET, "'Virtual' function on setting whether the object's image should be scaled smoothly or not."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_SMOOTH_SCALE_GET, "'Virtual' function on retrieving whether the object's image is to scaled smoothly or not."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_FILL_INSIDE_SET, "'Virtual' function on how to resize the object's internal image, when maintaining a given aspect ratio -- leave blank spaces or scale to fill all space, with pixels out of bounds."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_FILL_INSIDE_GET, "'Virtual' function on retrieving how the object's internal image is to be resized, when maintaining a given aspect ratio."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_FILL_OUTSIDE_SET, "Set if the image fills the entire object area, when keeping the aspect ratio."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_FILL_OUTSIDE_GET, "Get if the object is filled outside."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_OBJECT_GET, "Get the inlined image object of the image widget."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_LOAD_SIZE_SET, "'Virtual' function on setting the object's image loading size (in pixels, applied to both axis)."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_LOAD_SIZE_GET, "'Virtual' function on retrieving the object's image loading size."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_MEMFILE_SET, "Set a location in memory to be used as an image object's source bitmap."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ORIENT_SET, "Set the image orientation."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ORIENT_GET, "Get the image orientation."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_PRELOAD_DISABLED_SET, "Enable or disable preloading of the image."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_RESIZE_DOWN_SET, "'Virtual' function on setting whether the object's image can be resized to a size smaller than the original one."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_RESIZE_DOWN_GET, "'Virtual' function on retrieving whether the object's image can be resized to a size smaller than the original one."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_RESIZE_UP_SET, "'Virtual' function on setting whether the object's image can be resized to a size greater than the original one."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_RESIZE_UP_GET, "'Virtual' function on retrieving whether the object's image can be resized to a size greater than the original one."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_SCALE_SET, "'Virtual' function on setting the scale for the object's image size."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_SCALE_GET, "'Virtual' function on retrieving the scale for the object's image size."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_OBJECT_SIZE_GET, "Gets the current size of the image."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_SMOOTH_SET, "Set the smooth effect for an image."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_SMOOTH_GET, "Get the smooth effect for an image."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_NO_SCALE_SET, "Disable scaling of this object."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_NO_SCALE_GET, "Get whether scaling is disabled on the object."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_RESIZABLE_SET, "Set if the object is (up/down) resizable."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_RESIZABLE_GET, "Get if the object is (up/down) resizable."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_AVAILABLE_GET, "Get whether an image object supports animation or not."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_SET, "Set whether an image object (which supports animation) is to animate itself or not."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_GET, "Get whether an image object has animation enabled or not."),

     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_PLAY_SET, "Start or stop an image object's animation."),
     EO_OP_DESCRIPTION(ELM_OBJ_IMAGE_SUB_ID_ANIMATED_PLAY_GET, "Get whether an image object is under animation or not."),

     EO_OP_DESCRIPTION_SENTINEL
};
static const Eo_Class_Description class_desc = {
     EO_VERSION,
     MY_CLASS_NAME,
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(&ELM_OBJ_IMAGE_BASE_ID, op_desc, ELM_OBJ_IMAGE_SUB_ID_LAST),
     NULL,
     sizeof(Elm_Image_Smart_Data),
     _class_constructor,
     NULL
};
EO_DEFINE_CLASS(elm_obj_image_class_get, &class_desc, ELM_OBJ_WIDGET_CLASS, EVAS_SMART_CLICKABLE_INTERFACE, NULL);
