#include "CElmBasicWindow.h"

CElmBasicWindow::CElmBasicWindow(CEvasObject *parent, Local<Object> obj,
                                 Local<String> name, Local<Number> type)
   : CEvasObject()
{
   eo = elm_win_add(parent ? parent->get() : NULL,
                    *String::Utf8Value(name),
                    (Elm_Win_Type) (type->Value()));
   construct(eo, obj);

   /*
    * Create elements and attach to parent so children can see siblings
    * that have already been created.  Useful to find radio button groups.
    */
   Handle<Object> elements = Object::New();
   get_object()->Set(String::New("elements"), elements);
   realize_objects(obj->Get(String::New("elements")), elements);

   evas_object_focus_set(eo, 1);
   evas_object_smart_callback_add(eo, "delete,request", &on_delete, NULL);

   win_name = Persistent<Value>::New(name);
   win_type = Persistent<Value>::New(type);
}

Handle<Value> CElmBasicWindow::type_get(void) const
{
   return String::New("main");
}

Handle<Value> CElmBasicWindow::label_get() const
{
   return String::New(elm_win_title_get(eo));
}

void CElmBasicWindow::label_set(const char *str)
{
   elm_win_title_set(eo, str);
}

void CElmBasicWindow::on_delete(void *, Evas_Object *, void *)
{
   elm_exit();
}

void CElmBasicWindow::resize_set(Handle<Value>)
{
   ELM_ERR("warning: resize=true ignored on main window");
}
