#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_layout.h"
#include "elm_widget_label.h"

#include "Eo.h"

EAPI Eo_Op ELM_OBJ_LABEL_BASE_ID = EO_NOOP;

#define MY_CLASS ELM_OBJ_LABEL_CLASS

#define MY_CLASS_NAME "elm_label"

static const Elm_Layout_Part_Alias_Description _text_aliases[] =
{
   {"default", "elm.text"},
   {NULL, NULL}
};

static void
_recalc(void *data)
{
   ELM_LABEL_DATA_GET(data, sd);
   Elm_Widget_Smart_Data *wd = eo_data_get(data, ELM_OBJ_WIDGET_CLASS);

   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw;

   evas_event_freeze(evas_object_evas_get(data));
   evas_object_geometry_get
     (wd->resize_obj, NULL, NULL, &resw, NULL);
   if (sd->wrap_w > resw)
     resw = sd->wrap_w;

   edje_object_size_min_restricted_calc
     (wd->resize_obj, &minw, &minh, resw, 0);

   /* This is a hack to workaround the way min size hints are treated.
    * If the minimum width is smaller than the restricted width, it means
    * the mininmum doesn't matter. */
   if ((minw <= resw) && (minw != sd->wrap_w))
     {
        Evas_Coord ominw = -1;

        evas_object_size_hint_min_get(data, &ominw, NULL);
        minw = ominw;
     }

   evas_object_size_hint_min_set(data, minw, minh);
   evas_event_thaw(evas_object_evas_get(data));
   evas_event_thaw_eval(evas_object_evas_get(data));
}

static void
_label_format_set(Evas_Object *obj,
                  const char *format)
{
   if (format)
     edje_object_part_text_style_user_push(obj, "elm.text", format);
   else
     edje_object_part_text_style_user_pop(obj, "elm.text");
}

static void
_label_sliding_change(Evas_Object *obj)
{
   char *plaintxt;
   int plainlen = 0;

   ELM_LABEL_DATA_GET(obj, sd);
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   // doesn't support multiline sliding effect
   if (sd->linewrap)
     {
        sd->slidingmode = EINA_FALSE;
        return;
     }

   plaintxt = _elm_util_mkup_to_text
       (edje_object_part_text_get
         (wd->resize_obj, "elm.text"));
   if (plaintxt != NULL)
     {
        plainlen = strlen(plaintxt);
        free(plaintxt);
     }
   // too short to slide label
   if (plainlen < 1)
     {
        sd->slidingmode = EINA_TRUE;
        return;
     }

   if (sd->slidingmode)
     {
        Edje_Message_Float_Set *msg =
          alloca(sizeof(Edje_Message_Float_Set) + (sizeof(double)));

        if (sd->ellipsis)
          {
             sd->slidingellipsis = EINA_TRUE;
             elm_label_ellipsis_set(obj, EINA_FALSE);
          }

        msg->count = 1;
        msg->val[0] = sd->slide_duration;

        edje_object_message_send
          (wd->resize_obj, EDJE_MESSAGE_FLOAT_SET, 0, msg);
        edje_object_signal_emit
          (wd->resize_obj, "elm,state,slide,start", "elm");
     }
   else
     {
        edje_object_signal_emit
          (wd->resize_obj, "elm,state,slide,stop", "elm");
        if (sd->slidingellipsis)
          {
             sd->slidingellipsis = EINA_FALSE;
             elm_label_ellipsis_set(obj, EINA_TRUE);
          }
     }
}

static void
_elm_label_smart_theme(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   Eina_Bool int_ret = EINA_FALSE;

   Elm_Label_Smart_Data *sd = _pd;
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   evas_event_freeze(evas_object_evas_get(obj));

   eo_do_super(obj, elm_wdg_theme(&int_ret));
   if (!int_ret) goto end;

   _label_format_set(wd->resize_obj, sd->format);
   _label_sliding_change(obj);

end:
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
   if (ret) *ret = int_ret;

}

static void
_elm_label_smart_sizing_eval(Eo *obj, void *_pd EINA_UNUSED, va_list *list EINA_UNUSED)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, resh;

   ELM_LABEL_DATA_GET(obj, sd);
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   if (sd->linewrap)
     {
        evas_object_geometry_get
          (wd->resize_obj, NULL, NULL, &resw, &resh);
        if (resw == sd->lastw) return;
        sd->lastw = resw;
        _recalc(obj);
     }
   else
     {
        evas_event_freeze(evas_object_evas_get(obj));
        evas_object_geometry_get
          (wd->resize_obj, NULL, NULL, &resw, &resh);
        edje_object_size_min_calc
          (wd->resize_obj, &minw, &minh);
        if (sd->wrap_w > 0 && minw > sd->wrap_w) minw = sd->wrap_w;
        evas_object_size_hint_min_set(obj, minw, minh);
        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));
     }
}

static void
_on_label_resize(void *data,
                 Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   ELM_LABEL_DATA_GET(data, sd);

   if (sd->linewrap) elm_layout_sizing_eval(data);
}

static int
_get_value_in_key_string(const char *oldstring,
                         const char *key,
                         char **value)
{
   char *curlocater, *endtag;
   int firstindex = 0, foundflag = -1;

   curlocater = strstr(oldstring, key);
   if (curlocater)
     {
        int key_len = strlen(key);
        endtag = curlocater + key_len;
        if ((!endtag) || (*endtag != '='))
          {
             foundflag = 0;
             return -1;
          }
        firstindex = abs(oldstring - curlocater);
        firstindex += key_len + 1; // strlen("key") + strlen("=")
        *value = (char *)oldstring + firstindex;

        foundflag = 1;
     }
   else
     {
        foundflag = 0;
     }

   if (foundflag == 1) return 0;

   return -1;
}

static int
_strbuf_key_value_replace(Eina_Strbuf *srcbuf,
                          const char *key,
                          const char *value,
                          int deleteflag)
{
   char *kvalue;
   const char *srcstring = NULL;

   srcstring = eina_strbuf_string_get(srcbuf);

   if (_get_value_in_key_string(srcstring, key, &kvalue) == 0)
     {
        const char *val_end;
        int val_end_idx = 0;
        int key_start_idx = 0;
        val_end = strchr(kvalue, ' ');

        if (val_end)
          val_end_idx = val_end - srcstring;
        else
          val_end_idx = kvalue - srcstring + strlen(kvalue) - 1;

        /* -1 is because of the '=' */
        key_start_idx = kvalue - srcstring - 1 - strlen(key);
        eina_strbuf_remove(srcbuf, key_start_idx, val_end_idx);
        if (!deleteflag)
          {
             eina_strbuf_insert_printf(srcbuf, "%s=%s", key_start_idx, key,
                                       value);
          }
     }
   else if (!deleteflag)
     {
        if (*srcstring)
          {
             /* -1 because we want it before the ' */
             eina_strbuf_insert_printf
               (srcbuf, " %s=%s", eina_strbuf_length_get(srcbuf) - 1, key,
               value);
          }
        else
          {
             eina_strbuf_append_printf(srcbuf, "DEFAULT='%s=%s'", key, value);
          }
     }

   return 0;
}

static int
_stringshare_key_value_replace(const char **srcstring,
                               const char *key,
                               const char *value,
                               int deleteflag)
{
   Eina_Strbuf *sharebuf = NULL;

   sharebuf = eina_strbuf_new();
   eina_strbuf_append(sharebuf, *srcstring);
   _strbuf_key_value_replace(sharebuf, key, value, deleteflag);
   eina_stringshare_del(*srcstring);
   *srcstring = eina_stringshare_add(eina_strbuf_string_get(sharebuf));
   eina_strbuf_free(sharebuf);

   return 0;
}

static void
_elm_label_smart_text_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Label_Smart_Data *sd = _pd;
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);
   const char *item = va_arg(*list, const char *);
   const char *label = va_arg(*list, const char *);
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
   Eina_Bool int_ret = EINA_FALSE;

   if (!label) label = "";
   _label_format_set(wd->resize_obj, sd->format);

   eo_do_super(obj, elm_obj_layout_text_set(item, label, &int_ret));
   if (int_ret)
     {
        sd->lastw = 0;
        eo_do(obj, elm_obj_layout_sizing_eval());
     }
   if (ret) *ret = int_ret;
}

static void
_elm_label_smart_translate(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   evas_object_smart_callback_call(obj, "language,changed", NULL);

   if (ret) *ret = EINA_TRUE;
}

static char *
_access_info_cb(void *data __UNUSED__,
                Evas_Object *obj,
                Elm_Widget_Item *item __UNUSED__)
{
   const char *txt = elm_widget_access_info_get(obj);

   if (!txt) txt = elm_layout_text_get(obj, NULL);
   if (txt) return strdup(txt);

   return NULL;
}

static void
_elm_label_smart_add(Eo *obj, void *_pd, va_list *list EINA_UNUSED)
{
   Elm_Label_Smart_Data *priv = _pd;
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   eo_do_super(obj, evas_obj_smart_add());

   priv->linewrap = ELM_WRAP_NONE;
   priv->ellipsis = EINA_FALSE;
   priv->slidingmode = EINA_FALSE;
   priv->slidingellipsis = EINA_FALSE;
   priv->wrap_w = -1;
   priv->slide_duration = 10;

   priv->format = eina_stringshare_add("");
   _label_format_set(wd->resize_obj, priv->format);

   evas_object_event_callback_add
     (wd->resize_obj, EVAS_CALLBACK_RESIZE,
     _on_label_resize, obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);

   elm_layout_theme_set(obj, "label", "base", elm_widget_style_get(obj));
   elm_layout_text_set(obj, NULL, "<br>");

   _elm_access_object_register(obj, wd->resize_obj);
   _elm_access_text_set
     (_elm_access_object_get(obj), ELM_ACCESS_TYPE, E_("Label"));
   _elm_access_callback_set
     (_elm_access_object_get(obj), ELM_ACCESS_INFO, _access_info_cb, NULL);
   
   elm_layout_sizing_eval(obj);
}

EAPI Evas_Object *
elm_label_add(Evas_Object *parent)
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
         evas_obj_type_set(MY_CLASS_NAME));

   Evas_Object *parent = eo_parent_get(obj);
   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);
}

EAPI void
elm_label_line_wrap_set(Evas_Object *obj,
                        Elm_Wrap_Type wrap)
{
   ELM_LABEL_CHECK(obj);
   eo_do(obj, elm_obj_label_line_wrap_set(wrap));
}

static void
_line_wrap_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Wrap_Type wrap = va_arg(*list, Elm_Wrap_Type);
   const char *wrap_str, *text;
   int len;

   Elm_Label_Smart_Data *sd = _pd;
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   if (sd->linewrap == wrap) return;

   sd->linewrap = wrap;
   text = elm_layout_text_get(obj, NULL);
   if (!text) return;

   len = strlen(text);
   if (len <= 0) return;

   switch (wrap)
     {
      case ELM_WRAP_CHAR:
        wrap_str = "char";
        break;

      case ELM_WRAP_WORD:
        wrap_str = "word";
        break;

      case ELM_WRAP_MIXED:
        wrap_str = "mixed";
        break;

      default:
        wrap_str = "none";
        break;
     }

   if (_stringshare_key_value_replace(&sd->format, "wrap", wrap_str, 0) == 0)
     {
        _label_format_set(wd->resize_obj, sd->format);
        elm_layout_sizing_eval(obj);
     }
}

EAPI Elm_Wrap_Type
elm_label_line_wrap_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) EINA_FALSE;
   Elm_Wrap_Type ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_label_line_wrap_get(&ret));
   return ret;
}

static void
_line_wrap_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Wrap_Type *ret = va_arg(*list, Elm_Wrap_Type *);
   Elm_Label_Smart_Data *sd = _pd;
   *ret = sd->linewrap;
}

EAPI void
elm_label_wrap_width_set(Evas_Object *obj,
                         Evas_Coord w)
{
   ELM_LABEL_CHECK(obj);
   eo_do(obj, elm_obj_label_wrap_width_set(w));
}

static void
_wrap_width_set(Eo *obj, void *_pd, va_list *list)
{
   Evas_Coord w = va_arg(*list, Evas_Coord);
   Elm_Label_Smart_Data *sd = _pd;
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   if (w < 0) w = 0;

   if (sd->wrap_w == w) return;

   if (sd->ellipsis)
     _label_format_set(wd->resize_obj, sd->format);
   sd->wrap_w = w;

   elm_layout_sizing_eval(obj);
}

EAPI Evas_Coord
elm_label_wrap_width_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) 0;
   Evas_Coord ret = 0;
   eo_do((Eo *) obj, elm_obj_label_wrap_width_get(&ret));
   return ret;
}

static void
_wrap_width_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Evas_Coord *ret = va_arg(*list, Evas_Coord *);
   Elm_Label_Smart_Data *sd = _pd;
   *ret = sd->wrap_w;
}

EAPI void
elm_label_ellipsis_set(Evas_Object *obj,
                       Eina_Bool ellipsis)
{
   ELM_LABEL_CHECK(obj);
   eo_do(obj, elm_obj_label_ellipsis_set(ellipsis));
}

static void
_ellipsis_set(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool ellipsis = va_arg(*list, int);
   Eina_Strbuf *fontbuf = NULL;
   int len, removeflag = 0;
   const char *text;

   Elm_Label_Smart_Data *sd = _pd;
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   if (sd->ellipsis == ellipsis) return;
   sd->ellipsis = ellipsis;

   text = elm_layout_text_get(obj, NULL);
   if (!text) return;

   len = strlen(text);
   if (len <= 0) return;

   if (ellipsis == EINA_FALSE) removeflag = 1;  // remove fontsize tag

   fontbuf = eina_strbuf_new();
   eina_strbuf_append_printf(fontbuf, "%f", 1.0);

   if (_stringshare_key_value_replace
         (&sd->format, "ellipsis", eina_strbuf_string_get
           (fontbuf), removeflag) == 0)
     {
        _label_format_set(wd->resize_obj, sd->format);
        elm_layout_sizing_eval(obj);
     }
   eina_strbuf_free(fontbuf);
}

EAPI Eina_Bool
elm_label_ellipsis_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_label_ellipsis_get(&ret));
   return ret;
}

static void
_ellipsis_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   Elm_Label_Smart_Data *sd = _pd;
   *ret = sd->ellipsis;
}

EAPI void
elm_label_slide_set(Evas_Object *obj,
                    Eina_Bool slide)
{
   ELM_LABEL_CHECK(obj);
   eo_do(obj, elm_obj_label_slide_set(slide));
}

static void
_slide_set(Eo *obj, void *_pd, va_list *list)
{
   Eina_Bool slide = va_arg(*list, int);
   Elm_Label_Smart_Data *sd = _pd;

   if (sd->slidingmode == slide) return;
   sd->slidingmode = slide;

   _label_sliding_change(obj);
   elm_layout_sizing_eval(obj);
}

EAPI Eina_Bool
elm_label_slide_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_label_slide_get(&ret));
   return ret;
}

static void
_slide_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   Elm_Label_Smart_Data *sd = _pd;
   *ret = sd->slidingmode;
}

EAPI void
elm_label_slide_duration_set(Evas_Object *obj, double duration)
{
   ELM_LABEL_CHECK(obj);
   eo_do(obj, elm_obj_label_slide_duration_set(duration));
}

static void
_slide_duration_set(Eo *obj, void *_pd, va_list *list)
{
   double duration = va_arg(*list, double);
   Elm_Label_Smart_Data *sd = _pd;
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   Edje_Message_Float_Set *msg =
     alloca(sizeof(Edje_Message_Float_Set) + (sizeof(double)));

   sd->slide_duration = duration;
   msg->count = 1;
   msg->val[0] = sd->slide_duration;
   edje_object_message_send
     (wd->resize_obj, EDJE_MESSAGE_FLOAT_SET, 0, msg);
}

EAPI double
elm_label_slide_duration_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) 0.0;
   double ret = 0.0;
   eo_do((Eo *) obj, elm_obj_label_slide_duration_get(&ret));
   return ret;
}

static void
_slide_duration_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   double *ret = va_arg(*list, double *);
   Elm_Label_Smart_Data *sd = _pd;
   *ret = sd->slide_duration;
}

static void
_elm_label_smart_text_aliases_get(Eo *obj EINA_UNUSED, void *_pd EINA_UNUSED, va_list *list)
{
   const Elm_Layout_Part_Alias_Description **aliases = va_arg(*list, const Elm_Layout_Part_Alias_Description **);
   *aliases = _text_aliases;
}

static void
_elm_label_smart_focus_next_manager_is(Eo *obj EINA_UNUSED, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   *ret = EINA_FALSE;
}

static void
_elm_label_smart_focus_direction_manager_is(Eo *obj EINA_UNUSED, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   *ret = EINA_FALSE;
}

static void
_class_constructor(Eo_Class *klass)
{
   const Eo_Op_Func_Description func_desc[] = {
        EO_OP_FUNC(EO_BASE_ID(EO_BASE_SUB_ID_CONSTRUCTOR), _constructor),

        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_ADD), _elm_label_smart_add),

        EO_OP_FUNC(ELM_WIDGET_ID(ELM_WIDGET_SUB_ID_THEME), _elm_label_smart_theme),
        EO_OP_FUNC(ELM_WIDGET_ID(ELM_WIDGET_SUB_ID_TRANSLATE), _elm_label_smart_translate),
        EO_OP_FUNC(ELM_WIDGET_ID(ELM_WIDGET_SUB_ID_FOCUS_NEXT_MANAGER_IS), _elm_label_smart_focus_next_manager_is),
        EO_OP_FUNC(ELM_WIDGET_ID(ELM_WIDGET_SUB_ID_FOCUS_DIRECTION_MANAGER_IS), _elm_label_smart_focus_direction_manager_is),

        EO_OP_FUNC(ELM_OBJ_LAYOUT_ID(ELM_OBJ_LAYOUT_SUB_ID_SIZING_EVAL), _elm_label_smart_sizing_eval),
        EO_OP_FUNC(ELM_OBJ_LAYOUT_ID(ELM_OBJ_LAYOUT_SUB_ID_TEXT_SET), _elm_label_smart_text_set),
        EO_OP_FUNC(ELM_OBJ_LAYOUT_ID(ELM_OBJ_LAYOUT_SUB_ID_TEXT_ALIASES_GET), _elm_label_smart_text_aliases_get),

        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_LINE_WRAP_SET), _line_wrap_set),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_LINE_WRAP_GET), _line_wrap_get),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_WRAP_WIDTH_SET), _wrap_width_set),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_WRAP_WIDTH_GET), _wrap_width_get),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_ELLIPSIS_SET), _ellipsis_set),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_ELLIPSIS_GET), _ellipsis_get),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_SLIDE_SET), _slide_set),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_SLIDE_GET), _slide_get),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_SLIDE_DURATION_SET), _slide_duration_set),
        EO_OP_FUNC(ELM_OBJ_LABEL_ID(ELM_OBJ_LABEL_SUB_ID_SLIDE_DURATION_GET), _slide_duration_get),
        EO_OP_FUNC_SENTINEL
   };
   eo_class_funcs_set(klass, func_desc);
}
static const Eo_Op_Description op_desc[] = {
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_LINE_WRAP_SET, "Set the wrapping behavior of the label."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_LINE_WRAP_GET, "Get the wrapping behavior of the label."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_WRAP_WIDTH_SET, "Set wrap width of the label."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_WRAP_WIDTH_GET, "Get wrap width of the label."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_ELLIPSIS_SET, "Set the ellipsis behavior of the label."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_ELLIPSIS_GET, "Get the ellipsis behavior of the label."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_SLIDE_SET, "Set sliding effect of label widget."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_SLIDE_GET, "Get whether sliding effect is shown or not."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_SLIDE_DURATION_SET, "Set the slide duration (speed) of the label."),
     EO_OP_DESCRIPTION(ELM_OBJ_LABEL_SUB_ID_SLIDE_DURATION_GET, "Get the slide duration(speed) of the label."),
     EO_OP_DESCRIPTION_SENTINEL
};
static const Eo_Class_Description class_desc = {
     EO_VERSION,
     MY_CLASS_NAME,
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(&ELM_OBJ_LABEL_BASE_ID, op_desc, ELM_OBJ_LABEL_SUB_ID_LAST),
     NULL,
     sizeof(Elm_Label_Smart_Data),
     _class_constructor,
     NULL
};
EO_DEFINE_CLASS(elm_obj_label_class_get, &class_desc, ELM_OBJ_LAYOUT_CLASS, NULL);
