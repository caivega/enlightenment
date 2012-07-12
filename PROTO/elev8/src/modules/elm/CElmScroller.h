#ifndef C_ELM_SCROLLER_H
#define C_ELM_SCROLLER_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

using namespace v8;

class CElmScroller : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmScroller(Local<Object> _jsObject, CElmObject *parent);
   virtual ~CElmScroller();

   struct {
      Persistent<Value> content;
   } cached;

   static Handle<FunctionTemplate> GetTemplate();
   static Elm_Scroller_Policy policy_from_string(Handle<Value> val);
   static Local<Value> string_from_policy(Elm_Scroller_Policy policy);

   Persistent<Value> widget_base_theme;
public:
   static void Initialize(Handle<Object> val);

   void bounce_set(Handle<Value> val);
   Handle<Value> bounce_get() const;

   void policy_set(Handle<Value> val);
   Handle<Value> policy_get() const;

   void content_set(Handle<Value> val);
   Handle<Value> content_get() const;

   void widget_base_theme_set(Handle<Value> val);
   Handle<Value> widget_base_theme_get() const;

   void propagate_events_set(Handle<Value> val);
   Handle<Value> propagate_events_get() const;

   void horizontal_gravity_set(Handle<Value> val);
   Handle<Value> horizontal_gravity_get() const;

   void vertical_gravity_set(Handle<Value> val);
   Handle<Value> vertical_gravity_get() const;

   Handle<Value> region_get() const;

   Handle<Value> region_show(const Arguments& args);
   Handle<Value> page_bring_in(const Arguments& args);

   friend Handle<Value> CElmObject::New<CElmScroller>(const Arguments& args);
};

}

#endif // C_ELM_SCROLLER_H
