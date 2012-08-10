#include "CElmWeb.h"

namespace elm {

using namespace v8;

GENERATE_PROPERTY_CALLBACKS(CElmWeb, useragent);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, tab_propagate);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, uri);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, bg_color);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, text_matches_highlight);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, history_enabled);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, zoom);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, zoom_mode);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, inwin_mode);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, on_load_progress);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, on_title_change);
GENERATE_PROPERTY_CALLBACKS(CElmWeb, on_uri_change);
GENERATE_RO_PROPERTY_CALLBACKS(CElmWeb, forward_possible);
GENERATE_RO_PROPERTY_CALLBACKS(CElmWeb, title);
GENERATE_RO_PROPERTY_CALLBACKS(CElmWeb, selection);
GENERATE_RO_PROPERTY_CALLBACKS(CElmWeb, load_progress);
GENERATE_RO_PROPERTY_CALLBACKS(CElmWeb, back_possible);
GENERATE_METHOD_CALLBACKS(CElmWeb, popup_destroy);
GENERATE_METHOD_CALLBACKS(CElmWeb, text_search);
GENERATE_METHOD_CALLBACKS(CElmWeb, text_matches_mark);
GENERATE_METHOD_CALLBACKS(CElmWeb, text_matches_unmark_all);
GENERATE_METHOD_CALLBACKS(CElmWeb, stop);
GENERATE_METHOD_CALLBACKS(CElmWeb, reload);
GENERATE_METHOD_CALLBACKS(CElmWeb, reload_full);
GENERATE_METHOD_CALLBACKS(CElmWeb, back);
GENERATE_METHOD_CALLBACKS(CElmWeb, forward);
GENERATE_METHOD_CALLBACKS(CElmWeb, navigate);
GENERATE_METHOD_CALLBACKS(CElmWeb, region_show);
GENERATE_METHOD_CALLBACKS(CElmWeb, region_bring_in);
GENERATE_METHOD_CALLBACKS(CElmWeb, navigate_possible);

GENERATE_TEMPLATE_FULL(CElmObject, CElmWeb,
                       PROPERTY(useragent),
                       PROPERTY(tab_propagate),
                       PROPERTY(uri),
                       PROPERTY(bg_color),
                       PROPERTY(text_matches_highlight),
                       PROPERTY(history_enabled),
                       PROPERTY(zoom),
                       PROPERTY(zoom_mode),
                       PROPERTY(inwin_mode),
                       PROPERTY(on_title_change),
                       PROPERTY(on_uri_change),
                       PROPERTY(on_load_progress),
                       PROPERTY_RO(forward_possible),
                       PROPERTY_RO(title),
                       PROPERTY_RO(selection),
                       PROPERTY_RO(load_progress),
                       PROPERTY_RO(back_possible),
                       METHOD(popup_destroy),
                       METHOD(text_search),
                       METHOD(text_matches_mark),
                       METHOD(text_matches_unmark_all),
                       METHOD(stop),
                       METHOD(reload),
                       METHOD(reload_full),
                       METHOD(back),
                       METHOD(forward),
                       METHOD(navigate),
                       METHOD(region_show),
                       METHOD(region_bring_in),
                       METHOD(navigate_possible));

CElmWeb::CElmWeb(Local<Object> _jsObject, CElmObject *parent)
    : CElmObject(_jsObject,
                 elm_need_web() ? elm_web_add(parent->GetEvasObject()) : NULL)
{
}

void CElmWeb::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Web"), GetTemplate()->GetFunction());
}

Handle<Value> CElmWeb::useragent_get() const
{
   return String::New(elm_web_useragent_get(eo));
}

void CElmWeb::useragent_set(Handle<Value> val)
{
   if (val->IsString())
     elm_web_useragent_set(eo, *String::Utf8Value(val));
}

Handle<Value> CElmWeb::tab_propagate_get() const
{
   return Boolean::New(elm_web_tab_propagate_get(eo));
}

void CElmWeb::tab_propagate_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_web_tab_propagate_set(eo, val->BooleanValue());
}

Handle<Value> CElmWeb::uri_get() const
{
   return String::New(elm_web_uri_get(eo));
}

void CElmWeb::uri_set(Handle<Value> val)
{
   if (val->IsString())
     elm_web_uri_set(eo, *String::Utf8Value(val));
}

Handle<Value> CElmWeb::bg_color_get() const
{
   Handle<Value> bg_color = Object::New();
   int r, g, b, a;

   elm_web_bg_color_get(eo, &r, &g, &b, &a);
   bg_color->ToObject()->Set(String::NewSymbol("red"), Int32::New(r));
   bg_color->ToObject()->Set(String::NewSymbol("green"), Int32::New(g));
   bg_color->ToObject()->Set(String::NewSymbol("blue"), Int32::New(b));
   bg_color->ToObject()->Set(String::NewSymbol("alpha"), Int32::New(a));

   return bg_color;
}

void CElmWeb::bg_color_set(Handle<Value> val)
{
   if(!val->IsObject())
     return;

   int r = val->ToObject()->Get(String::NewSymbol("red"))->Int32Value();
   int g = val->ToObject()->Get(String::NewSymbol("green"))->Int32Value();
   int b = val->ToObject()->Get(String::NewSymbol("blue"))->Int32Value();
   int a = val->ToObject()->Get(String::NewSymbol("alpha"))->Int32Value();
   elm_web_bg_color_set(eo, r, g, b, a);
}

Handle<Value> CElmWeb::text_matches_highlight_get() const
{
   return Boolean::New(elm_web_text_matches_highlight_get(eo));
}

void CElmWeb::text_matches_highlight_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_web_text_matches_highlight_set(eo, val->BooleanValue());
}

Handle<Value> CElmWeb::history_enabled_get() const
{
   return Boolean::New(elm_web_history_enabled_get(eo));
}

void CElmWeb::history_enabled_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_web_history_enabled_set(eo, val->BooleanValue());
}

Handle<Value> CElmWeb::zoom_get() const
{
   return Number::New(elm_web_zoom_get(eo));
}

void CElmWeb::zoom_set(Handle<Value> val)
{
   if (!val->IsNumber())
     return;

   elm_web_zoom_set(eo, val->ToNumber()->Value());
}

Handle<Value> CElmWeb::zoom_mode_get() const
{
   return Number::New(elm_web_zoom_mode_get(eo));
}

void CElmWeb::zoom_mode_set(Handle<Value> val)
{
   if (!val->IsInt32() || (val->Int32Value() > ELM_WEB_ZOOM_MODE_LAST))
     return;

   elm_web_zoom_mode_set(eo, static_cast<Elm_Web_Zoom_Mode>(val->Int32Value()));
}

Handle<Value> CElmWeb::inwin_mode_get() const
{
   return Boolean::New(elm_web_inwin_mode_get(eo));
}

void CElmWeb::inwin_mode_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_web_inwin_mode_set(eo, val->BooleanValue());
}

Handle<Value> CElmWeb::on_load_progress_get() const
{
   return cb.on_load_progress;
}

void CElmWeb::on_load_progress_set(Handle<Value> val)
{
   if (!cb.on_load_progress.IsEmpty())
     {
        evas_object_smart_callback_del(eo,
                                       "load,progress",
                                       &OnLoadProgressWrapper);
        cb.on_load_progress.Dispose();
        cb.on_load_progress.Clear();
     }

   if (!val->IsFunction())
     return;

   cb.on_load_progress = Persistent<Value>::New(val);
   evas_object_smart_callback_add(eo,
                                  "load,progress",
                                  &OnLoadProgressWrapper,
                                  this);
}

void CElmWeb::OnLoadProgress(void *event_info)
{
   Handle<Value> progress =  Number::New(*static_cast<double *>(event_info));
   Handle<Function> callback(Function::Cast(*cb.on_load_progress));
   Handle<Value> args[1] = { progress };

   callback->Call(jsObject, 1, args);
}

void CElmWeb::OnLoadProgressWrapper(void *data, Evas_Object *, void *)
{
   CElmWeb *self = static_cast<CElmWeb *>(data);

   if (self->load_progress_get()->IsUndefined())
     return;

   double progress = self->load_progress_get()->NumberValue();

   self->OnLoadProgress(static_cast<void *>(&progress));
}

Handle<Value> CElmWeb::on_title_change_get() const
{
   return cb.on_title_change;
}

void CElmWeb::on_title_change_set(Handle<Value> val)
{
   if (!cb.on_title_change.IsEmpty())
     {
        evas_object_smart_callback_del(eo,
                                       "title,changed",
                                       &OnTitleChangeWrapper);
        cb.on_title_change.Dispose();
        cb.on_title_change.Clear();
     }

   if (!val->IsFunction())
     return;

   cb.on_title_change = Persistent<Value>::New(val);
   evas_object_smart_callback_add(eo,
                                  "title,changed",
                                  &OnTitleChangeWrapper,
                                  this);
}

void CElmWeb::OnTitleChange(void *event_info)
{
   Handle<Value> new_title = String::New(static_cast<const char*>(event_info));
   Handle<Function> callback(Function::Cast(*cb.on_title_change));
   Handle<Value> args[1] = { new_title };

   callback->Call(jsObject, 1, args);
}

void CElmWeb::OnTitleChangeWrapper(void *data, Evas_Object *, void *)
{
   CElmWeb *self = static_cast<CElmWeb *>(data);
   Handle<Value> current_title = self->title_get();

   if (current_title->IsUndefined())
     return;

   self->OnTitleChange(*String::Utf8Value(current_title->ToString()));
}

Handle<Value> CElmWeb::on_uri_change_get() const
{
   return cb.on_uri_change;
}

void CElmWeb::on_uri_change_set(Handle<Value> val)
{
   if (!cb.on_uri_change.IsEmpty())
     {
        evas_object_smart_callback_del(eo,
                                       "uri,changed",
                                       &OnUriChangeWrapper);
        cb.on_uri_change.Dispose();
        cb.on_uri_change.Clear();
     }

   if (!val->IsFunction())
     return;

   cb.on_uri_change = Persistent<Value>::New(val);
   evas_object_smart_callback_add(eo,
                                  "uri,changed",
                                  &OnUriChangeWrapper,
                                  this);
}

void CElmWeb::OnUriChange(void *event_info)
{
   Handle<Value> new_uri = String::New(static_cast<const char*>(event_info));
   Handle<Function> callback(Function::Cast(*cb.on_uri_change));
   Handle<Value> args[1] = { new_uri };

   callback->Call(jsObject, 1, args);
}

void CElmWeb::OnUriChangeWrapper(void *data, Evas_Object *, void *)
{
   CElmWeb *self = static_cast<CElmWeb *>(data);
   Handle<Value> current_uri = self->uri_get();

   if (current_uri->IsUndefined())
     return;

   self->OnUriChange(*String::Utf8Value(current_uri->ToString()));
}

Handle<Value> CElmWeb::forward_possible_get() const
{
   return Boolean::New(elm_web_forward_possible_get(eo));
}

Handle<Value> CElmWeb::title_get() const
{
   const char *current_title = elm_web_title_get(eo);

   return current_title ? String::New(current_title) : Undefined();
}

Handle<Value> CElmWeb::selection_get() const
{
   const char *current_selection = elm_web_selection_get(eo);

   return current_selection ? String::New(current_selection) : Undefined();
}

Handle<Value> CElmWeb::load_progress_get() const
{
   return Number::New(elm_web_load_progress_get(eo));
}

Handle<Value> CElmWeb::back_possible_get() const
{
   return Boolean::New(elm_web_back_possible_get(eo));
}

Handle<Value> CElmWeb::navigate_possible(const Arguments &args)
{
   if ((args.Length() < 1) || !args[0]->IsInt32())
     return Undefined();

   return Boolean::New(elm_web_navigate_possible_get(eo, (args[0]->Int32Value())));
}

Handle<Value> CElmWeb::popup_destroy(const Arguments &)
{
   return Boolean::New(elm_web_popup_destroy(eo));
}

Handle<Value> CElmWeb::text_search(const Arguments &args)
{
   if (args[0]->IsUndefined())
     return Undefined();

   const char *sentence = *String::Utf8Value(args[0]->ToString());
   Eina_Bool is_case_sensitive = args[1]->BooleanValue();
   Eina_Bool is_forward = args[2]->BooleanValue();
   Eina_Bool is_wrap = args[3]->BooleanValue();

   return Boolean::New(elm_web_text_search(eo,
                                           sentence,
                                           is_case_sensitive,
                                           is_forward,
                                           is_wrap));
}

Handle<Value> CElmWeb::text_matches_mark(const Arguments &args)
{
   if (args[0]->IsUndefined())
     return Undefined();

   const char *sentence = *String::Utf8Value(args[0]->ToString());
   Eina_Bool case_sensitive = args[1]->BooleanValue();
   Eina_Bool highlight = args[2]->BooleanValue();
   unsigned int limit = args[3]->Int32Value();

   return Boolean::New(elm_web_text_matches_mark(eo,
                                                 sentence,
                                                 case_sensitive,
                                                 highlight,
                                                 limit));
}

Handle<Value> CElmWeb::text_matches_unmark_all(const Arguments &)
{
   return Boolean::New(elm_web_text_matches_unmark_all(eo));
}

Handle<Value> CElmWeb::stop(const Arguments &)
{
   return Boolean::New(elm_web_stop(eo));
}

Handle<Value> CElmWeb::reload(const Arguments &)
{
   return Boolean::New(elm_web_reload(eo));
}

Handle<Value> CElmWeb::reload_full(const Arguments &)
{
   return Boolean::New(elm_web_reload_full(eo));
}

Handle<Value> CElmWeb::back(const Arguments &)
{
   return Boolean::New(elm_web_back(eo));
}

Handle<Value> CElmWeb::forward(const Arguments &)
{
   return Boolean::New(elm_web_forward(eo));
}

Handle<Value> CElmWeb::navigate(const Arguments &args)
{
   return Boolean::New(elm_web_navigate(eo, args[0]->Int32Value()));
}

Handle<Value> CElmWeb::region_show(const Arguments &args)
{
   elm_web_region_show(eo,
                       args[0]->Int32Value(),
                       args[1]->Int32Value(),
                       args[2]->Int32Value(),
                       args[3]->Int32Value());

   return Undefined();
}

Handle<Value> CElmWeb::region_bring_in(const Arguments &args)
{
   elm_web_region_bring_in(eo,
                           args[0]->Int32Value(),
                           args[1]->Int32Value(),
                           args[2]->Int32Value(),
                           args[3]->Int32Value());

   return Undefined();
}

}
