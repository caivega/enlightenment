#ifndef C_ELM_PHOTO_H
#define C_ELM_PHOTO_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

using namespace v8;

class CElmPhoto : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmPhoto(Local<Object> _jsObject, CElmObject *parent);
   static Handle<FunctionTemplate> GetTemplate();

public:
   static void Initialize(Handle<Object> target);

   Handle<Value> image_get() const;
   void image_set(Handle<Value> val);

   Handle<Value> size_get() const;
   void size_set(Handle<Value> val);

   Handle<Value> fill_get() const;
   void fill_set(Handle<Value> val);

   friend Handle<Value> CElmObject::New<CElmPhoto>(const Arguments& args);
};

}

#endif // C_ELM_PHOTO_H
