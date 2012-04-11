#ifndef C_ELM_CLOCK_H
#define C_ELM_CLOCK_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

using namespace v8;

class CElmClock : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmClock(Local<Object> _jsObject, CElmObject *parent);
   static Handle<FunctionTemplate> GetTemplate();

public:
   static void Initialize(Handle<Object> target);

   Handle<Value> show_am_pm_get() const;
   void show_am_pm_set(Handle<Value> val);

   Handle<Value> show_seconds_get() const;
   void show_seconds_set(Handle<Value> val);

   Handle<Value> hour_get() const;
   void hour_set(Handle<Value> val);

   Handle<Value> minute_get() const;
   void minute_set(Handle<Value> val);

   Handle<Value> second_get() const;
   void second_set(Handle<Value> val);

   Handle<Value> edit_get() const;
   void edit_set(Handle<Value> val);

   friend Handle<Value> CElmObject::New<CElmClock>(const Arguments& args);
};

}

#endif
