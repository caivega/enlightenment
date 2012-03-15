#include "CElmLabel.h"

CElmLabel::CElmLabel(CEvasObject *parent, Local<Object> obj)
   : CEvasObject()
   , prop_handler(property_list_base)
{
   eo = elm_label_add(parent->get());
   construct(eo, obj);
}

void CElmLabel::wrap_set(Handle<Value> wrap)
{
   if (wrap->IsNumber())
     elm_label_line_wrap_set(eo, static_cast<Elm_Wrap_Type>(wrap->Int32Value()));
}

Handle<Value> CElmLabel::wrap_get() const
{
   HandleScope scope;

   return scope.Close(Integer::New(elm_label_line_wrap_get(eo)));
}

PROPERTIES_OF(CElmLabel) = {
   PROP_HANDLER(CElmLabel, wrap),
   { NULL }
};
