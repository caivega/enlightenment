#include "elm.h"
#include "CElmSegment.h"

namespace elm {

using namespace v8;

GENERATE_PROPERTY_CALLBACKS(CElmSegment, items);

GENERATE_TEMPLATE(CElmSegment,
                  PROPERTY(items));

CElmSegment::CElmSegment(Local<Object> _jsObject, CElmObject *parent)
   : CElmObject(_jsObject, elm_segment_control_add(parent->GetEvasObject()))
{
}

CElmSegment::~CElmSegment()
{
   cached.items.Dispose();
}

void CElmSegment::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Segment"), GetTemplate()->GetFunction());
}

void CElmSegment::items_set(Handle<Value> val)
{
   if (!val->IsObject())
     {
        ELM_ERR("Expecting an object to set the segment item names");
        return;
     }

   Local<Object> in = val->ToObject();
   Local<Array> props = in->GetPropertyNames();
   for (unsigned int i = 0; i < props->Length(); i++)
     {
        Local<Value> item = in->Get(props->Get(Integer::New(i))->ToString());
        Local<Value> label;

        if (item->IsObject())
          label = item->ToObject()->Get(String::NewSymbol("label"));
        else if (item->IsString() || item->IsNumber())
          label = item->ToString();
        else
          {
             ELM_ERR("Invalid object type for segment label.");
             continue;
          }

        elm_segment_control_item_add(eo, NULL, *String::Utf8Value(label));
     }

   cached.items = Persistent<Value>::New(val);
}

Handle<Value> CElmSegment::items_get() const
{
   return cached.items;
}

}
