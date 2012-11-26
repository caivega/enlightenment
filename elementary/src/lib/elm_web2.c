#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_web.h"

#if defined(HAVE_ELEMENTARY_WEB) && defined(USE_WEBKIT2)
#include <EWebKit2.h>

EAPI Eo_Op ELM_OBJ_WEB_BASE_ID = EO_NOOP;

#define MY_CLASS ELM_OBJ_WEB_CLASS

#define MY_CLASS_NAME "elm_web"

static Ewk_View_Smart_Class _ewk_view_parent_sc =
  EWK_VIEW_SMART_CLASS_INIT_NULL;

static const Evas_Smart_Cb_Description _elm_web_smart_callbacks[] = {
   { "url,changed", "s" },
   { NULL, NULL }
};

static void
_view_smart_add(Evas_Object *obj)
{
   View_Smart_Data *sd;

   sd = calloc(1, sizeof(View_Smart_Data));
   evas_object_smart_data_set(obj, sd);

   _ewk_view_parent_sc.sc.add(obj);
}

static void
_view_smart_del(Evas_Object *obj)
{
   _ewk_view_parent_sc.sc.del(obj);
}

/**
 * Creates a new view object given the parent.
 *
 * @param parent object to use as parent.
 *
 * @return newly added Evas_Object or @c NULL on errors.
 */
Evas_Object *
_view_add(Evas_Object *parent)
{
   Evas *canvas = evas_object_evas_get(parent);
   static Evas_Smart *smart = NULL;
   Evas_Object *view;

   if (!smart)
     {
        static Ewk_View_Smart_Class api =
          EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION("EWK_View_Elementary");

        ewk_view_smart_class_set(&api);
        ewk_view_smart_class_set(&_ewk_view_parent_sc);

        // TODO: check every api method and provide overrides with hooks!
        // TODO: hooks should provide extension points
        // TODO: extension should have some kind of "default implementation",
        // TODO: that can be replaced or controlled by hooks.
        // TODO: ie: run_javascript_alert() should present an elm_win
        // TODO: by default, but user could override it to show as inwin.
        api.sc.add = _view_smart_add;
        api.sc.del = _view_smart_del;

        smart = evas_smart_class_new(&api.sc);
        if (!smart)
          {
             CRITICAL("Could not create smart class");
             return NULL;
          }
     }

   view = ewk_view_smart_add(canvas, smart, ewk_context_default_get());
   if (!view)
     {
        ERR("Could not create smart object object for view");
        return NULL;
     }

   return view;
}

static int _elm_need_web = 0;

EAPI Eina_Bool
elm_need_web(void)
{
   if (_elm_need_web++) return EINA_TRUE;
   ewk_init();
   return EINA_TRUE;
}

static void
_elm_web_smart_add(Eo *obj, void *_pd EINA_UNUSED, va_list *list EINA_UNUSED)
{
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

#ifdef HAVE_ELEMENTARY_WEB
   wd->resize_obj = _view_add(obj);

   eo_do_super(obj, evas_obj_smart_add());

   elm_widget_can_focus_set(obj, EINA_TRUE);
#endif
}

static void
_elm_web_smart_del(Eo *obj, void *_pd EINA_UNUSED, va_list *list EINA_UNUSED)
{
   eo_do_super(obj, evas_obj_smart_del());
}

EAPI Evas_Object *
elm_web_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);
   eo_unref(obj);
   return obj;
}

static void
_constructor(Eo *obj, void *_pd, va_list *list EINA_UNUSED)
{

   Elm_Web_Smart_Data *sd = _pd;
   sd->obj = obj;
   eo_do_super(obj, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME),
         evas_obj_smart_callbacks_descriptions_set(_elm_web_smart_callbacks, NULL));
   if (!elm_widget_sub_object_add(eo_parent_get(obj), obj))
     ERR("could not add %p as sub object of %p", obj, eo_parent_get(obj));
}

EAPI Evas_Object *
elm_web_webkit_view_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) NULL;
   Evas_Object *ret = NULL;
   eo_do((Eo *) obj, elm_obj_web_webkit_view_get(&ret));
   return ret;
}

static void
_webkit_view_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Evas_Object **ret = va_arg(*list, Evas_Object **);
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);
   *ret = wd->resize_obj;
}

EAPI void
elm_web_window_create_hook_set(Evas_Object *obj,
                               Elm_Web_Window_Open func,
                               void *data)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_window_create_hook_set(func, data));
}

static void
_window_create_hook_set(Eo *obj EINA_UNUSED, void *_pd EINA_UNUSED, va_list *list)
{
   Elm_Web_Window_Open func = va_arg(*list, Elm_Web_Window_Open);
   void *data = va_arg(*list, void *);
#ifdef HAVE_ELEMENTARY_WEB
  (void)func;
  (void)data;
#else
  (void)func;
  (void)data;
#endif
}

EAPI void
elm_web_dialog_alert_hook_set(Evas_Object *obj,
                              Elm_Web_Dialog_Alert func,
                              void *data)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_dialog_alert_hook_set(func, data));
}

static void
_dialog_alert_hook_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Web_Dialog_Alert func = va_arg(*list, Elm_Web_Dialog_Alert);
   void *data = va_arg(*list, void *);

#ifdef HAVE_ELEMENTARY_WEB
   (void)func;
   (void)data;
   (void)_pd;
#else
   (void)func;
   (void)data;
   (void)_pd;
#endif
}

EAPI void
elm_web_dialog_confirm_hook_set(Evas_Object *obj,
                                Elm_Web_Dialog_Confirm func,
                                void *data)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_dialog_confirm_hook_set(func, data));
}

static void
_dialog_confirm_hook_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Web_Dialog_Confirm func = va_arg(*list, Elm_Web_Dialog_Confirm);
   void *data = va_arg(*list, void *);

#ifdef HAVE_ELEMENTARY_WEB
   (void)func;
   (void)data;
   (void)_pd;
#else
   (void)func;
   (void)data;
   (void)_pd;
#endif
}

EAPI void
elm_web_dialog_prompt_hook_set(Evas_Object *obj,
                               Elm_Web_Dialog_Prompt func,
                               void *data)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_dialog_prompt_hook_set(func, data));
}

static void
_dialog_prompt_hook_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Web_Dialog_Prompt func = va_arg(*list, Elm_Web_Dialog_Prompt);
   void *data = va_arg(*list, void *);
#ifdef HAVE_ELEMENTARY_WEB
   (void)func;
   (void)data;
   (void)_pd;
#else
   (void)func;
   (void)data;
   (void)_pd;
#endif
}

EAPI void
elm_web_dialog_file_selector_hook_set(Evas_Object *obj,
                                      Elm_Web_Dialog_File_Selector func,
                                      void *data)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_dialog_file_selector_hook_set(func, data));
}

static void
_dialog_file_selector_hook_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Web_Dialog_File_Selector func = va_arg(*list, Elm_Web_Dialog_File_Selector);
   void *data = va_arg(*list, void *);
#ifdef HAVE_ELEMENTARY_WEB
   (void)func;
   (void)data;
   (void)_pd;
#else
   (void)func;
   (void)data;
   (void)_pd;
#endif
}

EAPI void
elm_web_console_message_hook_set(Evas_Object *obj,
                                 Elm_Web_Console_Message func,
                                 void *data)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_console_message_hook_set(func, data));
}

static void
_console_message_hook_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Web_Console_Message func = va_arg(*list, Elm_Web_Console_Message);
   void *data = va_arg(*list, void *);

#ifdef HAVE_ELEMENTARY_WEB
   (void)func;
   (void)data;
   (void)_pd;
#else
   (void)func;
   (void)data;
   (void)_pd;
#endif
}

EAPI void
elm_web_useragent_set(Evas_Object *obj,
                      const char *user_agent)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_useragent_set(user_agent));
}

static void
_useragent_set(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   const char *user_agent = va_arg(*list, const char *);
#ifdef HAVE_ELEMENTARY_WEB
   (void)user_agent;
   (void)obj;
#else
   (void)user_agent;
   (void)obj;
#endif
}

EAPI const char *
elm_web_useragent_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) NULL;
   const char *ret = NULL;
   eo_do((Eo *) obj, elm_obj_web_useragent_get(&ret));
   return ret;
}

static void
_useragent_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   const char **ret = va_arg(*list, const char **);

#ifdef HAVE_ELEMENTARY_WEB
   *ret = NULL;
   (void)obj;
#else
   *ret = NULL;
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_uri_set(Evas_Object *obj,
                const char *uri)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_uri_set(uri, &ret));
   return ret;
}

static void
_uri_set(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);

   const char *uri = va_arg(*list, const char *);
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
   Eina_Bool int_ret = EINA_FALSE;
   int_ret = ewk_view_url_set(wd->resize_obj, uri);
   if (ret) *ret = int_ret;
}

EAPI const char *
elm_web_uri_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) NULL;
   const char *ret = NULL;
   eo_do((Eo *) obj, elm_obj_web_uri_get(&ret));
   return ret;
}

static void
_uri_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   const char **ret = va_arg(*list, const char **);
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);
   *ret = ewk_view_url_get(wd->resize_obj);
}

EAPI const char *
elm_web_title_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) NULL;
   const char *ret = NULL;
   eo_do((Eo *) obj, elm_obj_web_title_get(&ret));
   return ret;
}

static void
_title_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   const char **ret = va_arg(*list, const char **);
   Elm_Widget_Smart_Data *wd = eo_data_get(obj, ELM_OBJ_WIDGET_CLASS);
   *ret = ewk_view_title_get(wd->resize_obj);
}

EAPI void
elm_web_bg_color_set(Evas_Object *obj,
                     int r,
                     int g,
                     int b,
                     int a)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_bg_color_set(r, g, b, a));
}


static void
_bg_color_set(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   int r = va_arg(*list, int);
   int g = va_arg(*list, int);
   int b = va_arg(*list, int);
   int a = va_arg(*list, int);
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
   (void)r;
   (void)g;
   (void)b;
   (void)a;
#else
   (void)obj;
   (void)r;
   (void)g;
   (void)b;
   (void)a;
#endif
}

EAPI void
elm_web_bg_color_get(const Evas_Object *obj,
                     int *r,
                     int *g,
                     int *b,
                     int *a)
{
   ELM_WEB_CHECK(obj);

   eo_do((Eo *) obj, elm_obj_web_bg_color_get(r, g, b, a));
}

static void
_bg_color_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   int *r = va_arg(*list, int *);
   int *g = va_arg(*list, int *);
   int *b = va_arg(*list, int *);
   int *a = va_arg(*list, int *);

   if (r) *r = 0;
   if (g) *g = 0;
   if (b) *b = 0;
   if (a) *a = 0;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI const char *
elm_web_selection_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) NULL;
   const char *ret = NULL;
   eo_do((Eo *) obj, elm_obj_web_selection_get(&ret));
   return ret;
}

static void
_selection_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   const char **ret = va_arg(*list, const char **);
#ifdef HAVE_ELEMENTARY_WEB
   *ret = NULL;
   (void)obj;
#else
   *ret = NULL;
   (void)obj;
#endif
}

EAPI void
elm_web_popup_selected_set(Evas_Object *obj,
                           int idx)
{
   ELM_WEB_CHECK(obj);

   eo_do(obj, elm_obj_web_popup_selected_set(idx));
}

static void
_popup_selected_set(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   int idx = va_arg(*list, int);
#ifdef HAVE_ELEMENTARY_WEB
   (void)idx;
   (void)obj;
#else
   (void)idx;
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_popup_destroy(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_popup_destroy(&ret));
   return ret;
}

static void
_popup_destroy(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_text_search(const Evas_Object *obj,
                    const char *string,
                    Eina_Bool case_sensitive,
                    Eina_Bool forward,
                    Eina_Bool wrap)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_web_text_search(string, case_sensitive, forward, wrap, &ret));
   return ret;
}

static void
_text_search(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   const char *string = va_arg(*list, const char *);
   Eina_Bool case_sensitive = va_arg(*list, int);
   Eina_Bool forward = va_arg(*list, int);
   Eina_Bool wrap = va_arg(*list, int);
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;

#ifdef HAVE_ELEMENTARY_WEB
   (void)string;
   (void)case_sensitive;
   (void)forward;
   (void)wrap;
   (void)obj;
#else
   (void)string;
   (void)case_sensitive;
   (void)forward;
   (void)wrap;
   (void)obj;
#endif
}

EAPI unsigned int
elm_web_text_matches_mark(Evas_Object *obj,
                          const char *string,
                          Eina_Bool case_sensitive,
                          Eina_Bool highlight,
                          unsigned int limit)
{
   ELM_WEB_CHECK(obj) 0;
   unsigned int ret = 0;
   eo_do(obj, elm_obj_web_text_matches_mark(string, case_sensitive, highlight, limit, &ret));
   return ret;
}

static void
_text_matches_mark(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   const char *string = va_arg(*list, const char *);
   Eina_Bool case_sensitive = va_arg(*list, int);
   Eina_Bool highlight = va_arg(*list, int);
   unsigned int limit = va_arg(*list, unsigned int);
   unsigned int *ret = va_arg(*list, unsigned int *);
   if (ret) *ret = 0;

#ifdef HAVE_ELEMENTARY_WEB
   (void)string;
   (void)case_sensitive;
   (void)highlight;
   (void)limit;
   (void)obj;
#else
   (void)string;
   (void)case_sensitive;
   (void)highlight;
   (void)limit;
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_text_matches_unmark_all(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;

   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_text_matches_unmark_all(&ret));
   return ret;
}

static void
_text_matches_unmark_all(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_text_matches_highlight_set(Evas_Object *obj,
                                   Eina_Bool highlight)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;

   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_text_matches_highlight_set(highlight, &ret));
   return ret;
}

static void
_text_matches_highlight_set(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool highlight = va_arg(*list, int);
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;

#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
   (void)highlight;
#else
   (void)obj;
   (void)highlight;
#endif
}

EAPI Eina_Bool
elm_web_text_matches_highlight_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *)obj, elm_obj_web_text_matches_highlight_get(&ret));
   return ret;
}

static void
_text_matches_highlight_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI double
elm_web_load_progress_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) - 1.0;
   double ret = - 1.0;
   eo_do((Eo *) obj, elm_obj_web_load_progress_get(&ret));
   return ret;
}

static void
_load_progress_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   double *ret = va_arg(*list, double *);
   *ret = -1.0;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_stop(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_stop(&ret));
   return ret;
}

static void
_stop(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_reload(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_reload(&ret));
   return ret;
}

static void
_reload(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_reload_full(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_reload_full(&ret));
   return ret;
}

static void
_reload_full(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_back(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_back(&ret));
   return ret;
}

static void
_back(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_forward(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_forward(&ret));
   return ret;
}

static void
_forward(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_navigate(Evas_Object *obj,
                 int steps)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_navigate(steps, &ret));
   return ret;
}

static void
_navigate(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   int steps = va_arg(*list, int);
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   if (ret) *ret = EINA_FALSE;

#ifdef HAVE_ELEMENTARY_WEB
   (void)steps;
   (void)obj;
#else
   (void)steps;
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_back_possible_get(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_back_possible_get(&ret));
   return ret;
}

static void
_back_possible_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_forward_possible_get(Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_forward_possible_get(&ret));
   return ret;
}

static void
_forward_possible_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_navigate_possible_get(Evas_Object *obj,
                              int steps)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do(obj, elm_obj_web_navigate_possible_get(steps, &ret));
   return ret;
}

static void
_navigate_possible_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   int steps = va_arg(*list, int);
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   *ret = EINA_FALSE;

#ifdef HAVE_ELEMENTARY_WEB
   (void)steps;
   (void)obj;
#else
   (void)steps;
   (void)obj;
#endif
}

EAPI Eina_Bool
elm_web_history_enabled_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_web_history_enabled_get(&ret));
   return ret;
}

static void
_history_enabled_get(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   *ret = EINA_FALSE;

#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
#else
   (void)obj;
#endif
}

EAPI void
elm_web_history_enabled_set(Evas_Object *obj,
                            Eina_Bool enable)
{
   ELM_WEB_CHECK(obj);
   eo_do(obj, elm_obj_web_history_enabled_set(enable));
}

static void
_history_enabled_set(Eo *obj, void *_pd EINA_UNUSED, va_list *list)
{
   Eina_Bool enable = va_arg(*list, int);
#ifdef HAVE_ELEMENTARY_WEB
   (void)enable;
   (void)obj;
#else
   (void)enable;
   (void)obj;
#endif
}

EAPI void
elm_web_zoom_set(Evas_Object *obj,
                 double zoom)
{
   ELM_WEB_CHECK(obj);
   eo_do(obj, elm_obj_web_zoom_set(zoom));
}

static void
_zoom_set(Eo *obj, void *_pd, va_list *list)
{
   double zoom = va_arg(*list, double);

#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
   (void)_pd;
   (void)zoom;
#else
   (void)obj;
   (void)_pd;
   (void)zoom;
#endif
}

EAPI double
elm_web_zoom_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) - 1.0;
   double ret = - 1.0;
   eo_do((Eo *) obj, elm_obj_web_zoom_get(&ret));
   return ret;
}

static void
_zoom_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   double *ret = va_arg(*list, double *);
   *ret = -1.0;
#ifdef HAVE_ELEMENTARY_WEB
   (void)_pd;
#else
   (void)_pd;
#endif
}

EAPI void
elm_web_zoom_mode_set(Evas_Object *obj,
                      Elm_Web_Zoom_Mode mode)
{
   ELM_WEB_CHECK(obj);
   eo_do(obj, elm_obj_web_zoom_mode_set(mode));
}

static void
_zoom_mode_set(Eo *obj, void *_pd, va_list *list)
{
   Elm_Web_Zoom_Mode mode = va_arg(*list, Elm_Web_Zoom_Mode);

#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
   (void)_pd;
   (void)mode;
#else
   (void)obj;
   (void)_pd;
   (void)mode;
#endif
}

EAPI Elm_Web_Zoom_Mode
elm_web_zoom_mode_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) ELM_WEB_ZOOM_MODE_LAST;
   Elm_Web_Zoom_Mode ret = ELM_WEB_ZOOM_MODE_LAST;
   eo_do((Eo *) obj, elm_obj_web_zoom_mode_get(&ret));
   return ret;
}

static void
_zoom_mode_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Elm_Web_Zoom_Mode *ret = va_arg(*list, Elm_Web_Zoom_Mode *);
   *ret = ELM_WEB_ZOOM_MODE_LAST;
#ifdef HAVE_ELEMENTARY_WEB
   (void)_pd;
#else
   (void)_pd;
#endif
}

EAPI void
elm_web_region_show(Evas_Object *obj,
                    int x,
                    int y,
                    int w __UNUSED__,
                    int h __UNUSED__)
{
   ELM_WEB_CHECK(obj);
   eo_do(obj, elm_obj_web_region_show(x, y, w, h));
}

static void
_region_show(Eo *obj, void *_pd, va_list *list)
{
   int x = va_arg(*list, int);
   int y = va_arg(*list, int);
   int w = va_arg(*list, int);
   int h = va_arg(*list, int);
   (void)w;
   (void)h;

#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
   (void)_pd;
   (void)x;
   (void)y;
#else
   (void)obj;
   (void)_pd;
   (void)x;
   (void)y;
#endif
}

EAPI void
elm_web_region_bring_in(Evas_Object *obj,
                        int x,
                        int y,
                        int w __UNUSED__,
                        int h __UNUSED__)
{
   ELM_WEB_CHECK(obj);
   eo_do(obj, elm_obj_web_region_bring_in(x, y, w, h));
}

static void
_region_bring_in(Eo *obj, void *_pd, va_list *list)
{
   int x = va_arg(*list, int);
   int y = va_arg(*list, int);
   int w = va_arg(*list, int);
   int h = va_arg(*list, int);
   (void)w;
   (void)h;

#ifdef HAVE_ELEMENTARY_WEB
   (void)obj;
   (void)_pd;
   (void)x;
   (void)y;
#else
   (void)obj;
   (void)_pd;
   (void)x;
   (void)y;
#endif
}

EAPI void
elm_web_inwin_mode_set(Evas_Object *obj,
                       Eina_Bool value)
{
   ELM_WEB_CHECK(obj);
   eo_do(obj, elm_obj_web_inwin_mode_set(value));
}

static void
_inwin_mode_set(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool value = va_arg(*list, int);
#ifdef HAVE_ELEMENTARY_WEB
   (void)_pd;
   (void)value;
#else
   (void)_pd;
   (void)value;
#endif
}

EAPI Eina_Bool
elm_web_inwin_mode_get(const Evas_Object *obj)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;
   eo_do((Eo *) obj, elm_obj_web_inwin_mode_get(&ret));
   return ret;
}

static void
_inwin_mode_get(Eo *obj EINA_UNUSED, void *_pd, va_list *list)
{
   Eina_Bool *ret = va_arg(*list, Eina_Bool *);
   *ret = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_WEB
   (void)_pd;
#else
   (void)_pd;
#endif
}

EAPI void
elm_web_window_features_ref(Elm_Web_Window_Features *wf)
{
   // FIXME : need to implement
   (void)wf;
}

EAPI void
elm_web_window_features_unref(Elm_Web_Window_Features *wf)
{
   // FIXME : need to implement
   (void)wf;
}

EAPI Eina_Bool
elm_web_window_features_property_get(const Elm_Web_Window_Features *wf,
                                     Elm_Web_Window_Feature_Flag flag)
{
   // FIXME : need to implement
   (void)wf;
   (void)flag;
   return EINA_FALSE;
}

EAPI void
elm_web_window_features_region_get(const Elm_Web_Window_Features *wf,
                                   Evas_Coord *x,
                                   Evas_Coord *y,
                                   Evas_Coord *w,
                                   Evas_Coord *h)
{
   // FIXME : need to implement
   (void)wf;
   (void)x;
   (void)y;
   (void)w;
   (void)h;
   return;
}
#endif


#if defined(HAVE_ELEMENTARY_WEB) && defined(USE_WEBKIT2)
static void
_class_constructor(Eo_Class *klass)
{
   const Eo_Op_Func_Description func_desc[] = {
        EO_OP_FUNC(EO_BASE_ID(EO_BASE_SUB_ID_CONSTRUCTOR), _constructor),

        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_ADD), _elm_web_smart_add),
        EO_OP_FUNC(EVAS_OBJ_SMART_ID(EVAS_OBJ_SMART_SUB_ID_DEL), _elm_web_smart_del),

        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_WEBKIT_VIEW_GET), _webkit_view_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_WINDOW_CREATE_HOOK_SET), _window_create_hook_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_DIALOG_ALERT_HOOK_SET), _dialog_alert_hook_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_DIALOG_CONFIRM_HOOK_SET), _dialog_confirm_hook_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_DIALOG_PROMPT_HOOK_SET), _dialog_prompt_hook_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_DIALOG_FILE_SELECTOR_HOOK_SET), _dialog_file_selector_hook_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_CONSOLE_MESSAGE_HOOK_SET), _console_message_hook_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_USERAGENT_SET), _useragent_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_USERAGENT_GET), _useragent_get),

        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_URI_SET), _uri_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_URI_GET), _uri_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_TITLE_GET), _title_get),

        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_BG_COLOR_SET), _bg_color_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_BG_COLOR_GET), _bg_color_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_SELECTION_GET), _selection_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_POPUP_SELECTED_SET), _popup_selected_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_POPUP_DESTROY), _popup_destroy),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_TEXT_SEARCH), _text_search),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_TEXT_MATCHES_MARK), _text_matches_mark),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_TEXT_MATCHES_UNMARK_ALL), _text_matches_unmark_all),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_TEXT_MATCHES_HIGHLIGHT_SET), _text_matches_highlight_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_TEXT_MATCHES_HIGHLIGHT_GET), _text_matches_highlight_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_LOAD_PROGRESS_GET), _load_progress_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_STOP), _stop),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_RELOAD), _reload),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_RELOAD_FULL), _reload_full),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_BACK), _back),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_FORWARD), _forward),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_NAVIGATE), _navigate),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_BACK_POSSIBLE_GET), _back_possible_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_FORWARD_POSSIBLE_GET), _forward_possible_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_NAVIGATE_POSSIBLE_GET), _navigate_possible_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_HISTORY_ENABLED_GET), _history_enabled_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_HISTORY_ENABLED_SET), _history_enabled_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_ZOOM_SET), _zoom_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_ZOOM_GET), _zoom_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_ZOOM_MODE_SET), _zoom_mode_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_ZOOM_MODE_GET), _zoom_mode_get),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_REGION_SHOW), _region_show),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_REGION_BRING_IN), _region_bring_in),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_INWIN_MODE_SET), _inwin_mode_set),
        EO_OP_FUNC(ELM_OBJ_WEB_ID(ELM_OBJ_WEB_SUB_ID_INWIN_MODE_GET), _inwin_mode_get),
        EO_OP_FUNC_SENTINEL
   };
   eo_class_funcs_set(klass, func_desc);
}
static const Eo_Op_Description op_desc[] = {
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_WEBKIT_VIEW_GET, "Get internal ewk_view object from web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_WINDOW_CREATE_HOOK_SET, "Sets the function to call when a new window is requested."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_DIALOG_ALERT_HOOK_SET, "Sets the function to call when an alert dialog."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_DIALOG_CONFIRM_HOOK_SET, "Sets the function to call when an confirm dialog."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_DIALOG_PROMPT_HOOK_SET, "Sets the function to call when an prompt dialog."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_DIALOG_FILE_SELECTOR_HOOK_SET, "Sets the function to call when an file selector dialog."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_CONSOLE_MESSAGE_HOOK_SET, "Sets the function to call when a console message is emitted from JS."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_USERAGENT_SET, "Change useragent of a elm_web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_USERAGENT_GET, "Return current useragent of elm_web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_TAB_PROPAGATE_GET, "Gets the status of the tab propagation."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_TAB_PROPAGATE_SET, "Sets whether to use tab propagation."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_URI_SET, "Sets the URI for the web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_URI_GET, "Gets the current URI for the object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_TITLE_GET, "Gets the current title."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_BG_COLOR_SET, "Sets the background color to be used by the web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_BG_COLOR_GET, "Gets the background color to be used by the web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_SELECTION_GET, "Gets a copy of the currently selected text."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_POPUP_SELECTED_SET, "Tells the web object which index in the currently open popup was selected."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_POPUP_DESTROY, "Dismisses an open dropdown popup."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_TEXT_SEARCH, "Searches the given string in a document."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_TEXT_MATCHES_MARK, "Marks matches of the given string in a document."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_TEXT_MATCHES_UNMARK_ALL, "Clears all marked matches in the document."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_TEXT_MATCHES_HIGHLIGHT_SET, "Sets whether to highlight the matched marks."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_TEXT_MATCHES_HIGHLIGHT_GET, "Gets whether highlighting marks is enabled."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_LOAD_PROGRESS_GET, "Gets the overall loading progress of the page."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_STOP, "Stops loading the current page."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_RELOAD, "Requests a reload of the current document in the object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_RELOAD_FULL, "Requests a reload of the current document, avoiding any existing caches."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_BACK, "Goes back one step in the browsing history."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_FORWARD, "Goes forward one step in the browsing history."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_NAVIGATE, "Jumps the given number of steps in the browsing history."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_BACK_POSSIBLE_GET, "Queries whether it's possible to go back in history."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_FORWARD_POSSIBLE_GET, "Queries whether it's possible to go forward in history."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_NAVIGATE_POSSIBLE_GET, "Queries whether it's possible to jump the given number of steps."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_HISTORY_ENABLED_GET, "Gets whether browsing history is enabled for the given object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_HISTORY_ENABLED_SET, "Enables or disables the browsing history."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_ZOOM_SET, "Sets the zoom level of the web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_ZOOM_GET, "Gets the current zoom level set on the web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_ZOOM_MODE_SET, "Sets the zoom mode to use."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_ZOOM_MODE_GET, "Gets the currently set zoom mode."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_REGION_SHOW, "Shows the given region in the web object."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_REGION_BRING_IN, "Brings in the region to the visible area."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_INWIN_MODE_SET, "Sets the default dialogs to use an Inwin instead of a normal window."),
     EO_OP_DESCRIPTION(ELM_OBJ_WEB_SUB_ID_INWIN_MODE_GET, "Gets whether Inwin mode is set for the current object."),
     EO_OP_DESCRIPTION_SENTINEL
};

static const Eo_Class_Description class_desc = {
     EO_VERSION,
     MY_CLASS_NAME,
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(&ELM_OBJ_WEB_BASE_ID, op_desc, ELM_OBJ_WEB_SUB_ID_LAST),
     NULL,
     sizeof(Elm_Web_Smart_Data),
     _class_constructor,
     NULL
};

EO_DEFINE_CLASS(elm_obj_web_class_get, &class_desc, ELM_OBJ_WIDGET_CLASS, NULL);
#endif
