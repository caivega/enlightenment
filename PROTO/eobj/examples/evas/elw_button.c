#include <Elementary.h>

#include "Eo.h"
#include "evas_obj.h"
#include "elw_button.h"

#include "config.h"

EAPI Eo_Op ELW_BUTTON_BASE_ID = 0;

EAPI const Eo_Event_Description _SIG_CLICKED =
        EO_EVENT_DESCRIPTION("clicked", "", "Called when there was a click.");

typedef struct
{
   Evas_Object *bt;
} Widget_Data;

#define MY_CLASS ELW_BUTTON_CLASS

static void
_position_set(Eo *obj, void *class_data EINA_UNUSED, va_list *list)
{
   (void) obj;
   Evas_Coord x, y;
   x = va_arg(*list, Evas_Coord);
   y = va_arg(*list, Evas_Coord);
   printf("But set position %d,%d\n", x, y);
   eo_do_super(obj, evas_obj_position_set(x, y));
}

static void
_text_set(Eo *obj EINA_UNUSED, void *class_data, va_list *list)
{
   Widget_Data *wd = class_data;
   const char *text;
   text = va_arg(*list, const char *);
   elm_object_text_set(wd->bt, text);
}

static void
_btn_clicked(void *data, Evas_Object *evas_obj, void *event_info)
{
   (void) evas_obj;
   (void) event_info;
   Eo *obj = data;
   eo_event_callback_call(obj, SIG_CLICKED, NULL);
}

static void
_constructor(Eo *obj, void *class_data)
{
   eo_constructor_super(obj);

   Widget_Data *wd = class_data;

   /* FIXME: An hack, because our tree is not yet only Eo */
   wd->bt = elm_button_add(eo_evas_object_get(eo_parent_get(obj)));
   evas_object_size_hint_align_set(wd->bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(wd->bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(wd->bt, "clicked", _btn_clicked, obj);

   eo_evas_object_set(obj, wd->bt);
}

static void
_destructor(Eo *obj, void *class_data EINA_UNUSED)
{
   eo_destructor_super(obj);

   //Widget_Data *wd = class_data;
   /* FIXME: Commented out because it's automatically done because our tree
    * is not made of only eo */
   //evas_object_del(wd->bt);
}

static void
_class_constructor(Eo_Class *klass)
{
   const Eo_Op_Func_Description func_desc[] = {
        EO_OP_FUNC(ELW_BUTTON_ID(ELW_BUTTON_SUB_ID_TEXT_SET), _text_set),
        EO_OP_FUNC(EVAS_OBJ_ID(EVAS_OBJ_SUB_ID_POSITION_SET), _position_set),
        EO_OP_FUNC_SENTINEL
   };

   eo_class_funcs_set(klass, func_desc);
}

static const Eo_Op_Description op_desc[] = {
     EO_OP_DESCRIPTION(ELW_BUTTON_SUB_ID_TEXT_SET, "s", "Text of a text supporting evas object."), // FIXME: This ID sholudn't really be defined here...
     EO_OP_DESCRIPTION_SENTINEL
};

static const Eo_Event_Description *event_desc[] = {
     SIG_CLICKED,
     NULL
};

static const Eo_Class_Description class_desc = {
     "Elw Button",
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(&ELW_BUTTON_BASE_ID, op_desc, ELW_BUTTON_SUB_ID_LAST),
     event_desc,
     sizeof(Widget_Data),
     _constructor,
     _destructor,
     _class_constructor,
     NULL
};

EO_DEFINE_CLASS(elw_button_class_get, &class_desc, EVAS_OBJ_CLASS, NULL)

