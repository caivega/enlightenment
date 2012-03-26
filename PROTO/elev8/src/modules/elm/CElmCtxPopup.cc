#include "CElmCtxPopup.h"

CElmCtxPopup::CElmCtxPopup(CEvasObject *par, Local<Object> obj) :
   CEvasObject(),
   prop_handler(property_list_base)
{
   // context popup is added to a given parent - not the top widget
   eo = elm_ctxpopup_add(par->get());
   construct(eo, obj);
   get_object()->Set(String::New("clear"), FunctionTemplate::New(clear)->GetFunction());
   get_object()->Set(String::New("dismiss"), FunctionTemplate::New(dismiss)->GetFunction());
   get_object()->Set(String::New("append"), FunctionTemplate::New(append)->GetFunction());
}

Handle<Value> CElmCtxPopup::hover_parent_get() const
{
   return parent;
}

void CElmCtxPopup::hover_parent_set(Handle<Value> val)
{
   if (val->IsObject())
     {
        parent.Dispose();
        parent = Persistent<Value>::New(val);
        CEvasObject *p = make_or_get(this, val);

        if (p)
          {
             elm_ctxpopup_hover_parent_set(eo, p->get());
          }
        else
          printf("Failed Setting parent\n");

     }
}

Handle<Value> CElmCtxPopup::horizontal_get() const
{
   return Boolean::New(elm_ctxpopup_horizontal_get(eo));
}

void CElmCtxPopup::horizontal_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_ctxpopup_horizontal_set(eo, val->BooleanValue());
}

Handle<Value> CElmCtxPopup::direction_priority_get() const
{
   Elm_Ctxpopup_Direction f, s, q, t;
   elm_ctxpopup_direction_priority_get(eo, &f, &s, &t, &q);
   Local<Object> obj = Object::New();
   obj->Set(String::New("first"), Number::New(f));
   obj->Set(String::New("second"), Number::New(s));
   obj->Set(String::New("third"), Number::New(t));
   obj->Set(String::New("fourth"), Number::New(q));
   return obj;
}

void CElmCtxPopup::direction_priority_set(Handle<Value> val)
{
   if (val->IsObject())
     {
        HandleScope handle_scope;
        if (!val->IsObject())
          return;
        Local<Object> obj = val->ToObject();
        Local<Value> first  = obj->Get(String::New("first"));
        Local<Value> second  = obj->Get(String::New("second"));
        Local<Value> third  = obj->Get(String::New("third"));
        Local<Value> fourth  = obj->Get(String::New("fourth"));
        if (!first->IsNumber() || !second->IsNumber() || !third->IsNumber() || !fourth->IsNumber())
          return;

        Elm_Ctxpopup_Direction f=(Elm_Ctxpopup_Direction)first->NumberValue();
        Elm_Ctxpopup_Direction s=(Elm_Ctxpopup_Direction)second->NumberValue();
        Elm_Ctxpopup_Direction t=(Elm_Ctxpopup_Direction)third->NumberValue();
        Elm_Ctxpopup_Direction q=(Elm_Ctxpopup_Direction)fourth->NumberValue();

        elm_ctxpopup_direction_priority_set(eo, f, s, t, q);
     }
}

Handle<Value> CElmCtxPopup::direction_get() const
{
   return Number::New(elm_ctxpopup_direction_get(eo));
}

void CElmCtxPopup::direction_set(Handle<Value>)
{
   // do nothing
   // ctx popup direction is set based on priority
}

void CElmCtxPopup::on_dismissed(void *)
{
   Handle<Object> obj = get_object();
   HandleScope handle_scope;
   Handle<Value> val = on_dismissed_val;
   assert(val->IsFunction());
   Handle<Function> fn(Function::Cast(*val));
   Handle<Value> args[1] = { obj };
   fn->Call(obj, 1, args);
}

void CElmCtxPopup::eo_on_dismissed(void *data, Evas_Object *, void *event_info)
{
   CElmCtxPopup *dismissed = static_cast<CElmCtxPopup*>(data);
   dismissed->on_dismissed(event_info);
}

void CElmCtxPopup::on_dismissed_set(Handle<Value> val)
{
   on_dismissed_val.Dispose();
   on_dismissed_val = Persistent<Value>::New(val);
   if (val->IsFunction())
     evas_object_smart_callback_add(eo, "dismissed", &eo_on_dismissed, this);
   else
     evas_object_smart_callback_del(eo, "dismissed", &eo_on_dismissed);
}

Handle<Value> CElmCtxPopup::on_dismissed_get(void) const
{
   return on_dismissed_val;
}

Handle<Value> CElmCtxPopup::dismiss(const Arguments& args)
{
   CEvasObject *self = eo_from_info(args.This());
   CElmCtxPopup *ctxpopup = static_cast<CElmCtxPopup *>(self);
   elm_ctxpopup_dismiss(ctxpopup->get());
   return Undefined();
}

Handle<Value> CElmCtxPopup::clear(const Arguments& args)
{
   CEvasObject *self = eo_from_info(args.This());
   CElmCtxPopup *ctxpopup = static_cast<CElmCtxPopup *>(self);
   elm_ctxpopup_clear(ctxpopup->get());
   return Undefined();
}

void CElmCtxPopup::sel(void *data, Evas_Object *, void *)
{
    CtxPopupItemClass *itc = (CtxPopupItemClass *)data;
    if (itc->on_select.IsEmpty())
        return;
    Handle<Function> fn(Function::Cast(*(itc->on_select)));
    Local<Object> temp = Object::New();
    temp->Set(String::New("data"), itc->data);
    fn->Call(temp, 0, 0);
}

Handle<Value> CElmCtxPopup::append(const Arguments& args)
{
   CEvasObject *self = eo_from_info(args.This());
   CElmCtxPopup *ctxpopup = static_cast<CElmCtxPopup *>(self);
   if (args[0]->IsObject())
     {
        CtxPopupItemClass *itc = new CtxPopupItemClass();

        Local<Object> obj = args[0]->ToObject();
        String::Utf8Value str(obj->Get(String::New("label")));
        itc->label = std::string(*str);
        itc->icon = make_or_get(ctxpopup->get_parent(), obj->Get(String::New("icon")));
        itc->on_select = Persistent<Value>::New(obj->Get(String::New("on_select")));
        itc->data = Persistent<Value>::New(obj->Get(String::New("data")));

        elm_ctxpopup_item_append(ctxpopup->get(), *str, itc->icon->get(), sel, itc);
    }
   return Undefined();
}

PROPERTIES_OF(CElmCtxPopup) = 
{
     PROP_HANDLER(CElmCtxPopup, hover_parent),
     PROP_HANDLER(CElmCtxPopup, direction_priority),
     PROP_HANDLER(CElmCtxPopup, direction),
     PROP_HANDLER(CElmCtxPopup, horizontal),
     PROP_HANDLER(CElmCtxPopup, on_dismissed),
     { NULL, NULL, NULL },
};
