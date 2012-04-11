#include "elm.h"
#include "CElmBox.h"

namespace elm {

using namespace v8;

GENERATE_PROPERTY_CALLBACKS(CElmBox, horizontal);
GENERATE_PROPERTY_CALLBACKS(CElmBox, homogeneous);

GENERATE_TEMPLATE(CElmBox,
                  PROPERTY(horizontal),
                  PROPERTY(homogeneous));

CElmBox::CElmBox(Local <Object> _jsObject, CElmObject *parent)
   : CElmObject(_jsObject, elm_box_add(parent->GetEvasObject()))
{
}

void CElmBox::horizontal_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_box_horizontal_set(eo, val->BooleanValue());
}

Handle<Value> CElmBox::horizontal_get() const
{
   return Boolean::New(elm_box_horizontal_get(eo));
}

void CElmBox::homogeneous_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_box_homogeneous_set(eo, val->BooleanValue());
}

Handle<Value> CElmBox::homogeneous_get() const
{
   return Boolean::New(elm_box_homogeneous_get(eo));
}

void CElmBox::DidRealiseElement(Local<Value> obj)
{
   elm_box_pack_end(eo, GetEvasObjectFromJavascript<CElmBox>(obj));
}

void CElmBox::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Box"), GetTemplate()->GetFunction());
}

}
