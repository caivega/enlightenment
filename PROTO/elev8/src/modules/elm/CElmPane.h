#ifndef C_ELM_PANE_H
#define C_ELM_PANE_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

class CElmPane : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmPane(Local<Object> _jsObject, CElmObject *parent);
   virtual ~CElmPane();

   struct {
      Persistent<Value> left;
      Persistent<Value> right;
   } cached;

   static Handle<FunctionTemplate> GetTemplate();

public:
   static void Initialize(Handle<Object> target);

   Handle<Value> horizontal_get() const;
   void horizontal_set(Handle<Value> val);

   Handle<Value> left_get() const;
   void left_set(Handle<Value> val);

   Handle<Value> right_get() const;
   void right_set(Handle<Value> val);

   friend Handle<Value> CElmObject::New<CElmPane>(const Arguments& args);
};

}

#endif

