#include "CElmBox.h"

CElmBox::CElmBox(CEvasObject *parent, Local<Object> obj)
   : CEvasObject()
   , prop_handler(property_list_base)
{
   eo = elm_box_add(parent->top_widget_get());
   construct(eo, obj);

   /*
    * Create elements and attach to parent so children can see siblings
    * that have already been created.  Useful to find radio button groups.
    */
   Handle <Object> elements = Object::New();
   get_object()->Set(String::New("elements"), elements);
   realize_objects(obj->Get(String::New("elements")), elements);
}

void CElmBox::horizontal_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_box_horizontal_set(eo, val->BooleanValue());
}

Handle<Value> CElmBox::horizontal_get() const
{
   HandleScope scope;

   return scope.Close(Boolean::New(elm_box_horizontal_get(eo)));
}

void CElmBox::homogeneous_set(Handle<Value> val)
{
   if (val->IsBoolean())
     elm_box_homogeneous_set(eo, val->BooleanValue());
}

Handle<Value> CElmBox::homogeneous_get() const
{
   HandleScope scope;

   return scope.Close(Boolean::New(elm_box_homogeneous_get(eo)));
}

void CElmBox::add_child(CEvasObject *child)
{
   elm_box_pack_end(eo, child->get());
}

CEvasObject *CElmBox::get_child(Handle<Value> name)
{
   Handle<Object> obj = get_object();
   Local<Value> elements_val = obj->Get(String::New("elements"));

   if (!elements_val->IsObject())
     {
        ELM_ERR("elements not an object");
        return NULL;
     }

   Local<Object> elements = elements_val->ToObject();
   Local<Value> val = elements->Get(name);

   if (val->IsObject())
     return eo_from_info(val->ToObject());

   ELM_ERR("value %s not an object", *String::Utf8Value(val->ToString()));
   return NULL;
}

PROPERTIES_OF(CElmBox) = {
   PROP_HANDLER(CElmBox, horizontal),
   PROP_HANDLER(CElmBox, homogeneous),
   { NULL }
};
