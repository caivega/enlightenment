#include "CElmGrid.h"

CElmGrid::CElmGrid(CEvasObject *parent, Local<Object> obj)
   : CEvasObject()
   , prop_handler(property_list_base)
{
   eo = elm_grid_add(parent->top_widget_get());
   construct(eo, obj);

   get_object()->Set(String::NewSymbol("add"), FunctionTemplate::New(add)->GetFunction());
   get_object()->Set(String::NewSymbol("clear"), FunctionTemplate::New(clear)->GetFunction());

   items_set(obj->Get(String::New("subobjects")));
}

Handle<Value> CElmGrid::add(const Arguments& args)
{
   CElmGrid *grid = static_cast<CElmGrid *>(eo_from_info(args.This()));

   if (args[0]->IsObject())
     grid->pack_set(args[0]);
   return Undefined();
}

Handle<Value> CElmGrid::clear(const Arguments& args)
{
   CElmGrid *grid = static_cast<CElmGrid *>(eo_from_info(args.This()));

   elm_grid_clear(grid->get(), true);
   grid->grid_items.clear();
   return Undefined();
}

void CElmGrid::items_set(Handle<Value> val)
{
   if (!val->IsObject())
     {
        ELM_ERR( "not an object!");
        return;
     }

   Local<Object> in = val->ToObject();
   Local<Array> props = in->GetPropertyNames();

   /* iterate through elements and instantiate them */
   for (unsigned int i = 0; i < props->Length(); i++)
     {
        Local<Value> value = props->Get(Integer::New(i));
        pack_set(in->Get(value->ToString()));
     }
}

void CElmGrid::pack_set(Handle<Value> item)
{
   CEvasObject *child = NULL;

   if (!item->IsObject())
     {
        // FIXME: permit adding strings here?
        ELM_ERR("grid item is not an object");
        return;
     }

   Local<Value> subobj = item->ToObject()->Get(String::New("subobject"));
   if (!subobj->IsObject())
     return;

   //TODO : need to check if this is an existing child.
   child = make_or_get(this, subobj);
   if (!child)
      return;

   Local<Value> xpos = item->ToObject()->Get(String::New("x"));
   Local<Value> ypos = item->ToObject()->Get(String::New("y"));
   Local<Value> width = item->ToObject()->Get(String::New("w"));
   Local<Value> height = item->ToObject()->Get(String::New("h"));

   if (!xpos->IsNumber() || !ypos->IsNumber() || !width->IsNumber() ||
       !height->IsNumber())
     {
        ELM_ERR("Coordinates not set or not a number? x=%d, y=%d, w=%d or h=%d",
                xpos->IsNumber(), ypos->IsNumber(), width->IsNumber(),
                height->IsNumber());
        return;
     }

   elm_grid_pack(this->get(), child->get(),
                 xpos->IntegerValue(), ypos->IntegerValue(),
                 width->IntegerValue(), height->IntegerValue());
   grid_items.push_back(child);
}

Handle<Value> CElmGrid::pack_get() const
{
   return Undefined();
}

void CElmGrid::size_set(Handle<Value> val)
{
   int x, y;
   if (get_xy_from_object(val, x, y))
     {
        ELM_INF("Grid Size = %d %d", x,y);
        elm_grid_size_set(eo, x, y);
     }
}

Handle<Value> CElmGrid::size_get() const
{
   HandleScope scope;

   int x, y;
   elm_grid_size_get (eo, &x, &y);

   Local<Object> obj = Object::New();
   obj->Set(String::New("x"), Number::New(x));
   obj->Set(String::New("y"), Number::New(y));

   return scope.Close(obj);
}

PROPERTIES_OF(CElmGrid) = {
   PROP_HANDLER(CElmGrid, size),
   PROP_HANDLER(CElmGrid, pack),
   { NULL }
};

