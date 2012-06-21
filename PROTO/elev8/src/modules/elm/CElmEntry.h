#ifndef C_ELM_ENTRY_H
#define C_ELM_ENTRY_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

class CElmEntry : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmEntry(Local<Object> _jsObject, CElmObject *parent);
   ~CElmEntry();

   static Handle<FunctionTemplate> GetTemplate();

   struct {
      Persistent<Value> on_change;
   } cb;

public:
   static void Initialize(Handle<Object> val);

   Handle<Value> on_change_get() const;
   void on_change_set(Handle<Value> value);
   static void OnChangeWrapper(void *data, Evas_Object *, void *);
   void OnChange();

   Handle<Value> password_get() const;
   void password_set(Handle<Value> value);

   Handle<Value> editable_get() const;
   void editable_set(Handle<Value> value);

   Handle<Value> line_wrap_get() const;
   void line_wrap_set(Handle<Value> value);

   Handle<Value> scrollable_get() const;
   void scrollable_set(Handle<Value> value);

   Handle<Value> single_line_get() const;
   void single_line_set(Handle<Value> value);

   Handle<Value> entry_get() const;
   void entry_set(Handle<Value> value);
   void entry_append(Handle<Value> value);
   void entry_insert(Handle<Value> value);

   Handle<Value> cursor_pos_get() const;
   void cursor_pos_set(Handle<Value> value);

   Handle<Value> cursor_begin_get() const;
   void cursor_begin_set(Handle<Value>);

   Handle<Value> cursor_end_get() const;
   void cursor_end_set(Handle<Value>);

   Handle<Value> is_empty_get() const;

   Handle<Value> selection_get() const;

   Handle<Value> cursor_content_get() const;

   Handle<Value> cursor_next(const Arguments&);
   Handle<Value> cursor_prev(const Arguments&);
   Handle<Value> cursor_up(const Arguments&);
   Handle<Value> cursor_down(const Arguments&);

   friend Handle<Value> CElmObject::New<CElmEntry>(const Arguments& args);
};

}

#endif
