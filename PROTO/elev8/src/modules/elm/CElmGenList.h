#ifndef C_ELM_GEN_LIST_H
#define C_ELM_GEN_LIST_H

#include <string>
#include <v8.h>
#include "CEvasObject.h"

class CElmGenList : public CEvasObject {
   FACTORY(CElmGenList)
protected:
   CPropHandler<CElmGenList> prop_handler;

   struct GenListItemClass {
       Persistent<Value> type;
       Persistent<Value> data;
       Persistent<Value> on_text;
       Persistent<Value> on_content;
       Persistent<Value> on_state;
       Persistent<Value> on_delete;
       Persistent<Value> on_select;
       std::string item_style;
       Elm_Genlist_Item_Class eitc;
       CElmGenList *genlist;
   };

   static char *text_get(void *data, Evas_Object *, const char *part);
   static Evas_Object *content_get(void *data, Evas_Object *, const char *part);
   static Eina_Bool state_get(void *, Evas_Object *, const char *);
   static void del(void *data, Evas_Object *);
   static void sel(void *data, Evas_Object *, void *);
   static bool get_hv_from_object(Handle<Value> val, bool &h, bool &v);

public:
   CElmGenList(CEvasObject *parent, Local<Object> obj);

   static Handle<Value> clear(const Arguments& args);
   static Handle<Value> append(const Arguments& args);

   virtual Handle<Value> multi_select_get() const;
   virtual void multi_select_set(Handle<Value> value);

   virtual Handle<Value> reorder_mode_get() const;
   virtual void reorder_mode_set(Handle<Value> value);

   virtual Handle<Value> mode_get() const;
   virtual void mode_set(Handle<Value> value);

   virtual Handle<Value> select_mode_get() const;
   virtual void select_mode_set(Handle<Value> value);

   virtual Handle<Value> block_count_get() const;
   virtual void block_count_set(Handle<Value> value);

   virtual Handle<Value>longpress_timeout_get() const;
   virtual void longpress_timeout_set(Handle<Value> value);

   virtual void bounce_set(Handle<Value> val);
   virtual Handle<Value> bounce_get() const;

   virtual Handle<Value> highlight_mode_get() const;
   virtual void highlight_mode_set(Handle<Value> value);

   virtual Handle<Value> tree_effect_enabled_get() const;
   virtual void tree_effect_enabled_set(Handle<Value> value);

};

#endif

