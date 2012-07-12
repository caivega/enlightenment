#ifndef C_ELM_TOOLBAR_H
#define C_ELM_TOOLBAR_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

using namespace v8;

class CElmToolbar : public CElmObject {
private:
   class Item {
   public:
      Persistent<Object> self;
      Persistent<Value> data;
      Persistent<Value> callback;

      Item(Handle<Object> _self, Handle<Value> _data, Handle<Value> _callback)
         : self(Persistent<Object>::New(_self))
         , data(Persistent<Value>::New(_data))
         , callback(Persistent<Value>::New(_callback)) {}
      ~Item()
        {
           data.Dispose();
           callback.Dispose();
           self.Dispose();
        }
   };

   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmToolbar(Local<Object> _jsObject, CElmObject *parent);
   static Handle<FunctionTemplate> GetTemplate();

public:
   static void Initialize(Handle<Object> target);

   virtual void DidRealiseElement(Local<Value> val);
   Handle<Value> append(const Arguments& args);
   void append(Local<Value> icon, Local<Value> label, Local<Value> data,
               Local<Value> callback);

   static void OnSelect(void *data, Evas_Object *, void *);

   Handle<Value> icon_size_get() const;
   void icon_size_set(Handle<Value> value);

   Handle<Value> icon_order_lookup_get() const;
   void icon_order_lookup_set(Handle<Value> value);

   Handle<Value> homogeneous_get() const;
   void homogeneous_set(Handle<Value> value);

   Handle<Value> align_get() const;
   void align_set(Handle<Value> value);

   Handle<Value> shrink_mode_get() const;
   void shrink_mode_set(Handle<Value> value);

   Handle<Value> horizontal_get() const;
   void horizontal_set(Handle<Value> value);

   friend Handle<Value> CElmObject::New<CElmToolbar>(const Arguments& args);
};

}

#endif
