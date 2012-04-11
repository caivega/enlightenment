#ifndef C_ELM_SEGMENT_H
#define C_ELM_SEGMENT_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

class CElmSegment : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmSegment(Local<Object> _jsObject, CElmObject *parent);
   virtual ~CElmSegment();

   struct {
      Persistent<Value> items;
   } cached;

   static Handle<FunctionTemplate> GetTemplate();

public:
   static void Initialize(Handle<Object> target);

   Handle<Value> items_get() const;
   void items_set(Handle<Value> val);

   friend Handle<Value> CElmObject::New<CElmSegment>(const Arguments& args);
};

}

#endif
