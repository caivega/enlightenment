#include <Elementary.h>

#include "Eo.h"
#include "evas_obj.h"
#include "elw_win.h"

typedef struct
{
   Evas_Object *win;
   Evas_Object *bg;
} Widget_Data;

#define MY_CLASS ELW_WIN_CLASS

static void
my_win_del(void *data, Evas_Object *obj, void *event_info)
{
   /* called when my_win_main is requested to be deleted */
   elm_exit(); /* exit the program's main loop that runs in elm_run() */
   (void) data;
   (void) obj;
   (void) event_info;
}

static void
_constructor(Eo *obj, void *class_data)
{
   eo_constructor_super(obj);

   Widget_Data *wd = class_data;

   /* FIXME: Will actually do something about those when I care... */
   wd->win = elm_win_add(NULL, "eo-test", ELM_WIN_BASIC);
   elm_win_title_set(wd->win, "Eo Test");
   elm_win_autodel_set(wd->win, EINA_TRUE);
   evas_object_smart_callback_add(wd->win, "delete,request", my_win_del, NULL);

   wd->bg = elm_bg_add(wd->win);
   elm_win_resize_object_add(wd->win, wd->bg);
   evas_object_size_hint_weight_set(wd->bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(wd->bg);

   eo_evas_object_set(obj, wd->win);
}

static const Eo_Class_Description class_desc = {
     "Elw Win",
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(NULL, NULL, 0),
     NULL,
     sizeof(Widget_Data),
     _constructor,
     NULL,
     NULL,
     NULL
};


EO_DEFINE_CLASS(elw_win_class_get, &class_desc, EVAS_OBJ_CLASS, NULL)

