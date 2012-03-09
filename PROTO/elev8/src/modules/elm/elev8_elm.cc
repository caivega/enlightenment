/*
 * elev8 - javascript for EFL
 *
 * The script's job is to prepare for the main loop to run
 * then exit
 */

#include <list>
#include <map>
#include <string>

#include "elev8_elm.h"
#include "CEvasObject.h"
#include "CEvasImage.h"
#include "CElmBasicWindow.h"
#include "CElmButton.h"
#include "CElmLayout.h"
#include "CElmBackground.h"
#include "CElmRadio.h"
#include "CElmBox.h"
#include "CElmLabel.h"
#include "CElmFlip.h"
#include "CElmActionSlider.h"
#include "CElmIcon.h"
#include "CElmScroller.h"
#include "CElmSlider.h"
#include "CElmImage.h"

int elev8_elm_log_domain = -1;

using namespace v8;

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

public:
   CElmGenList(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_genlist_add(parent->top_widget_get());
        construct(eo, obj);
        get_object()->Set(String::New("append"), FunctionTemplate::New(append)->GetFunction());
        get_object()->Set(String::New("clear"), FunctionTemplate::New(clear)->GetFunction());
     }


   /* GenList functions that are going to do the heavy weight lifting */
   static char *text_get(void *data, Evas_Object *, const char *part)
     {
        GenListItemClass *itc = (GenListItemClass *)data;
        Handle<Function> fn(Function::Cast(*(itc->on_text)));
        Local<Object> temp = Object::New();
        temp->Set(String::New("part"), String::New(part));
        temp->Set(String::New("data"), itc->data);
        Handle<Value> args[1] = { temp };
        Local<Value> text = fn->Call(temp, 1, args);

        if (text->IsString())
          {
             String::Utf8Value str(text->ToString());
             return strdup(*str);
          }
        else
          return NULL;
     }

   static Evas_Object *content_get(void *data, Evas_Object *, const char *part)
     {
        GenListItemClass *itc = (GenListItemClass *)data;
        Handle<Function> fn(Function::Cast(*(itc->on_content)));
        Local<Object> temp = Object::New();
        temp->Set(String::New("part"), String::New(part));
        temp->Set(String::New("data"), itc->data);
        Handle<Value> args[1] = { temp };
        Local<Value> retval = fn->Call(temp, 1, args);

        if (retval->IsObject())
          {
             CEvasObject *content = make_or_get(itc->genlist, retval->ToObject());
             if (content)
               return content->get();
          }

        return NULL;
     }

   static Eina_Bool state_get(void *, Evas_Object *, const char *)
     {
        return EINA_TRUE;
     }

   static void del(void *data, Evas_Object *)
     {
        delete static_cast<GenListItemClass *>(data);
     }

   static void sel(void *data, Evas_Object *, void *)
     {
        GenListItemClass *itc = (GenListItemClass *)data;
        if (itc->on_select.IsEmpty())
          return;
        Handle<Function> fn(Function::Cast(*(itc->on_select)));
        Local<Object> temp = Object::New();
        temp->Set(String::New("data"), itc->data);
        fn->Call(temp, 0, 0);
     }
   /* End of GenList functions */

   static Handle<Value> clear(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        elm_genlist_clear(static_cast<CElmGenList *>(self)->get());
        return Undefined();
     }

   static Handle<Value> append(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmGenList *genlist = static_cast<CElmGenList *>(self);
        if (args[0]->IsObject())
          {
             GenListItemClass *itc = new GenListItemClass();

             Local<Object> obj = args[0]->ToObject();
             itc->type = Persistent<Value>::New(obj->Get(String::New("type")));
             itc->on_text = Persistent<Value>::New(obj->Get(String::New("on_text")));
             itc->on_content = Persistent<Value>::New(obj->Get(String::New("on_content")));
             itc->on_state = Persistent<Value>::New(obj->Get(String::New("on_state")));
             itc->on_select = Persistent<Value>::New(obj->Get(String::New("on_select")));
             itc->data = Persistent<Value>::New(obj->Get(String::New("data")));
             //TODO : Check genlist class type and modify or add
			 
             itc->eitc.item_style = "default";
             itc->eitc.func.text_get = text_get;
             itc->eitc.func.content_get = content_get;
             itc->eitc.func.state_get = state_get;
             itc->eitc.func.del = del;
             itc->genlist = genlist;
             elm_genlist_item_append(genlist->get(), &itc->eitc, itc, NULL,
                                        ELM_GENLIST_ITEM_NONE,
                                        sel, itc);
          }
        return Undefined();
     }

   virtual Handle<Value> multi_select_get() const
     {
        return Boolean::New(elm_genlist_multi_select_get(eo));
     }

   virtual void multi_select_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_genlist_multi_select_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> reorder_mode_get() const
     {
        return Boolean::New(elm_genlist_reorder_mode_get(eo));
     }

   virtual void reorder_mode_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_genlist_reorder_mode_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> height_for_width_mode_get() const
     {
        return Boolean::New(elm_genlist_height_for_width_mode_get(eo));
     }

   virtual void height_for_width_mode_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_genlist_height_for_width_mode_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> compress_mode_get() const
     {
        return Boolean::New(elm_genlist_compress_mode_get(eo));
     }

   virtual void compress_mode_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_genlist_compress_mode_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> no_select_mode_get() const
     {
        return Boolean::New(elm_genlist_no_select_mode_get(eo));
     }

   virtual void no_select_mode_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_genlist_no_select_mode_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> always_select_mode_get() const
     {
        return Boolean::New(elm_genlist_always_select_mode_get(eo));
     }

   virtual void always_select_mode_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_genlist_always_select_mode_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> block_count_get() const
     {
        return Number::New(elm_genlist_block_count_get(eo));
     }

   virtual void block_count_set(Handle<Value> value)
     {
        if (value->IsNumber())
          elm_genlist_block_count_set(eo, value->IntegerValue());
     }

   virtual Handle<Value>longpress_timeout_get() const
     {
        return Number::New(elm_genlist_longpress_timeout_get(eo));
     }

   virtual void longpress_timeout_set(Handle<Value> value)
     {
        if (value->IsNumber())
          elm_genlist_longpress_timeout_set(eo, value->IntegerValue());
     }
};

template<> CEvasObject::CPropHandler<CElmGenList>::property_list
CEvasObject::CPropHandler<CElmGenList>::list[] = {
  PROP_HANDLER(CElmGenList, multi_select),
  PROP_HANDLER(CElmGenList, always_select_mode),
  PROP_HANDLER(CElmGenList, no_select_mode),
  PROP_HANDLER(CElmGenList, compress_mode),
  PROP_HANDLER(CElmGenList, reorder_mode),
  PROP_HANDLER(CElmGenList, height_for_width_mode),
  PROP_HANDLER(CElmGenList, block_count),
  PROP_HANDLER(CElmGenList, longpress_timeout),
  { NULL, NULL, NULL },
};

class CElmList : public CEvasObject {
   FACTORY(CElmList)
protected:
   class Item {
   public:
     Local<Value> on_clicked;
     Handle<Value> label;
     Handle<Value> icon;
     bool disabled;
   };

   class ListItem : public Item {
   public:
     CEvasObject  *icon_left;
     CEvasObject  *icon_right;
     Elm_Object_Item *li;
     Handle<Value> end;
     Handle<Value> tooltip;
   };

protected:
   Persistent<Value> items;
   CPropHandler<CElmList> prop_handler;
   std::list<ListItem*> list;

   const static int LABEL = 1;
   const static int ICON = 2;
   const static int END = 3;
   const static int TOOLTIP = 4;

public:
   CElmList(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_list_add(parent->top_widget_get());

        construct(eo, obj);
        items_set(obj->Get(String::New("items")));

        get_object()->Set(String::New("append"), FunctionTemplate::New(append)->GetFunction());

        get_object()->Set(String::New("prepend"), FunctionTemplate::New(prepend)->GetFunction());
        get_object()->Set(String::New("get_label"), FunctionTemplate::New(get_label)->GetFunction());
        get_object()->Set(String::New("get_icon"), FunctionTemplate::New(get_icon)->GetFunction());
        get_object()->Set(String::New("get_end"), FunctionTemplate::New(get_end)->GetFunction());
        get_object()->Set(String::New("get_tooltip"), FunctionTemplate::New(get_tooltip)->GetFunction());
        get_object()->Set(String::New("set_label"), FunctionTemplate::New(set_label)->GetFunction());
        get_object()->Set(String::New("set_icon"), FunctionTemplate::New(set_icon)->GetFunction());
        get_object()->Set(String::New("set_end"), FunctionTemplate::New(set_end)->GetFunction());
        get_object()->Set(String::New("set_tooltip"), FunctionTemplate::New(set_tooltip)->GetFunction());
        get_object()->Set(String::New("insert_after"), FunctionTemplate::New(insert_after)->GetFunction());
        get_object()->Set(String::New("insert_before"), FunctionTemplate::New(insert_before)->GetFunction());
        get_object()->Set(String::New("selected_item_get"), FunctionTemplate::New(selected_item_get)->GetFunction());
        get_object()->Set(String::New("selected_item_set"), FunctionTemplate::New(selected_item_set)->GetFunction());
        get_object()->Set(String::New("del"), FunctionTemplate::New(del)->GetFunction());
        get_object()->Set(String::New("num_items"), FunctionTemplate::New(num_items)->GetFunction());
        get_object()->Set(String::New("disabled"), FunctionTemplate::New(num_items)->GetFunction());

     }

   static Handle<Value> append(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmList *list = static_cast<CElmList *>(self);
        if (args[0]->IsObject())
          {
             list->new_item_set(-1, args[0]);
          }
        return Undefined();
     }
   static Handle<Value> prepend(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmList *list = static_cast<CElmList *>(self);
        if (args[0]->IsObject())
          {
             list->new_item_set(0, args[0]);
          }
        return Undefined();
     }
   static Handle<Value> disabled(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmList *list = static_cast<CElmList *>(self);
        if (!list->list.empty() && args[0]->IsNumber() && args[1]->IsBoolean())
          {
              unsigned int val = args[0]->IntegerValue();

              if (val <= list->list.size())
                {
                   std::list<ListItem*>::iterator i = list->list.begin();
                   for ( ; val>0; val--)
                     i++;
                   elm_object_item_disabled_set((*i)->li, args[1]->BooleanValue());
                }
          }
        return Undefined();
     }
   static Handle<Value> get_label(const Arguments& args)
     {
         return get_item(LABEL, args);
     }
   static Handle<Value> get_icon(const Arguments& args)
     {
         return get_item(ICON, args);
     }
   static Handle<Value> get_end(const Arguments& args)
     {
         return get_item(END, args);
     }
   static Handle<Value> get_tooltip(const Arguments& args)
     {
         return get_item(TOOLTIP, args);
     }
   static Handle<Value> get_item(int field, const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmList *list = static_cast<CElmList *>(self);
        if (!list->list.empty() && args[0]->IsNumber())
          {
              unsigned int val = args[0]->IntegerValue();
              if (val <= list->list.size())
                {
                   std::list<ListItem*>::iterator i = list->list.begin();

                   for (;val>0; val--)
                     i++;

                   switch(field)
                     {
                        case LABEL:
                           return (*i)->label;
                        case ICON:
                           return (*i)->icon;
                        case END:
                           return (*i)->end;
                        case TOOLTIP:
                           return (*i)->tooltip;
                        default:
                           return Undefined();
                     }
                }
          }
        return Undefined();
     }

   static Handle<Value> set_label(const Arguments& args)
     {
         return set_item(LABEL, args);
     }
   static Handle<Value> set_icon(const Arguments& args)
     {
         return set_item(ICON, args);
     }
   static Handle<Value> set_end(const Arguments& args)
     {
         return set_item(END, args);
     }
   static Handle<Value> set_tooltip(const Arguments& args)
     {
         return set_item(TOOLTIP, args);
     }
   static Handle<Value> set_item(int field, const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmList *list = static_cast<CElmList *>(self);
        if (!list->list.empty() && args[0]->IsNumber())
          {
             unsigned int val = args[0]->IntegerValue();
             if (val <= list->list.size())
               {
                  std::list<ListItem*>::iterator i = list->list.begin();

                  for (;val>0; val--)
                    i++;
                  ListItem *it = *i;

                  switch(field)
                    {
                       case LABEL:
                          if ( args[1]->IsString())
                            {
                               static_cast<Persistent<Value> >(it->label).Dispose();
                               it->label = v8::Persistent<Value>::New(args[1]->ToString());
                               String::Utf8Value str(it->label->ToString());
                               elm_object_item_text_set(it->li, *str);
                           }
                           break;
                       case ICON:
                          if ( args[1]->IsObject())
                            {
                               static_cast<Persistent<Value> >(it->icon).Dispose();
                               it->icon = v8::Persistent<Value>::New(args[1]);
                               it->icon_left = make_or_get(list, it->icon);
                               if (it->icon_left)
                                 {
                                    elm_icon_scale_set(it->icon_left->get(), 0, 0);
                                    evas_object_size_hint_align_set(it->icon_left->get(), 0.0, 0.0);
                                    elm_object_item_part_content_set(it->li, NULL, it->icon_left->get());
                                 }
                            }
                          break;
                       case END:
                          if ( args[1]->IsObject())
                            {
                               static_cast<Persistent<Value> >(it->end).Dispose();
                               it->end = v8::Persistent<Value>::New(args[1]);
                               it->icon_right = make_or_get(list, it->end);

                               if (it->icon_right)
                                 {
                                    elm_icon_scale_set(it->icon_right->get(), 0, 0);
                                    evas_object_size_hint_align_set(it->icon_right->get(), 0.0, 0.0);
                                    elm_object_item_part_content_set(it->li, "end", it->icon_right->get());
                                 }
                            }
                          break;
                       case TOOLTIP:
                          if (args[1]->IsString())
                            {
                               static_cast<Persistent<Value> >(it->tooltip).Dispose();
                               it->tooltip = v8::Persistent<Value>::New(args[1]->ToString());
                               String::Utf8Value str(it->tooltip->ToString());
                               elm_object_tooltip_text_set(elm_object_item_widget_get(it->li), *str);
                            }
                          break;
                       default:
                          return Undefined();
                    }
                  elm_list_go(list->get());
               }
          }
        return Undefined();
     }
   static Handle<Value> insert_after(const Arguments&)
     {
        return Undefined();
     }
   static Handle<Value> insert_before(const Arguments&)
     {
        return Undefined();
     }
   static Handle<Value> selected_item_get(const Arguments&)
     {
        return Undefined();
     }
   static Handle<Value> selected_item_set(const Arguments&)
     {
        return Undefined();
     }
   static Handle<Value> del(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmList *list = static_cast<CElmList *>(self);

        if (!list->list.empty() && args[0]->IsNumber())
          {
             int val = args[0]->IntegerValue();

             if (val == -1) // delete last one
               val = list->list.size();

             if (val < (int) list->list.size())
               {
                  std::list<ListItem*>::iterator i = list->list.begin();

                  for (; val > 0; val--)
                    i++;

                  ListItem *it = *i;

                  elm_object_item_del(it->li);
                  elm_list_go(list->get());

                  list->list.erase(i);

                  delete it;
               }
          }
        return Undefined();
     }
   static Handle<Value> num_items(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmList *list = static_cast<CElmList *>(self);
        return v8::Number::New(list->list.size());
     }

   static void eo_on_click(void *data, Evas_Object *, void *)
     {
       if (data)
         {
            ListItem *it = static_cast<ListItem *>(data);

            if (*it->on_clicked != NULL)
              {
                 if (it->on_clicked->IsFunction())
                   {
                      Handle<Function> fn(Function::Cast(*(it->on_clicked)));
                      Local<Object> obj = Object::New();
                      obj->Set(String::New("label"), it->label);
                      obj->Set(String::New("icon"), it->icon);
                      obj->Set(String::New("end"), it->end);
                      obj->Set(String::New("tooltip"), it->tooltip);
                      fn->Call(obj, 0, NULL);
                   }
              }
         }
     }

   virtual Handle<Value> items_get(void) const
     {
        return items;
     }

   virtual void items_set(Handle<Value> val)
     {
        if (!val->IsObject())
          {
             ELM_ERR( "not an object!");
             return;
          }

        Local<Object> in = val->ToObject();
        Local<Array> props = in->GetPropertyNames();

        items.Dispose();
        items = Persistent<Value>::New(val);

        /* iterate through elements and instantiate them */
        // there can be no elements in the list
        for (unsigned int i = 0; i < props->Length(); i++)
          {
             Local<Value> x = props->Get(Integer::New(i));
             String::Utf8Value val(x);

             Local<Value> item = in->Get(x->ToString());

             // -1 means end of list
             ListItem *it = new_item_set(-1, item);
             if (it!=NULL)
             {
                ELM_INF( "New list item added.");
             }
          }

     }

   /*
    * -1 = end of list
    *  >=0 appropriate index
    */

   virtual ListItem * new_item_set(int pos, Handle<Value> item)
     {
        if (!item->IsObject())
          {
             // FIXME: permit adding strings here?
             ELM_ERR( "list item is not an object");
             return NULL;
          }

        if (items==Null())
          {
             ELM_ERR( "Please add atleast empty \"items\" to list");
             return NULL;
          }

        ListItem *it = list.front();
        if ((pos == 0) || (pos==-1)) //seperate insert and removal case
          {
             it = new ListItem();

             it->label = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("label")));
             it->icon = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("icon")));
             it->end = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("end")));
             it->tooltip = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("tooltip")));

             if ( !it->label->IsString() && !it->icon->IsObject()
                      && !it->end->IsObject())
               {

                  ELM_ERR( "Basic elements missing");
                  delete it;
                  return NULL;
               }
          }

        if (-1 == pos)
          {
             // either a label with icon
             it->li = elm_list_item_append(eo,NULL,NULL,NULL,&eo_on_click,(void*)it);
             list.push_back(it);
          }
        else if (0 == pos)
          {
             // either a label with icon
             it->li = elm_list_item_prepend(eo,NULL,NULL,NULL,&eo_on_click,(void*)it);
             list.push_front(it);
          }
        else
          {
             // get the Eina_List
             const Eina_List *iter = elm_list_items_get (get());
             std::list<ListItem*>::iterator i = list.begin();

             for (;pos>0; pos--)
               {
                  i++;
                  iter = iter->next;
               }
             it->label = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("label")));
             it->icon = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("icon")));
             it->end = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("end")));
             it->tooltip = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("tooltip")));
             list.insert(i, it);
          }

        if ( it->label->IsString())
          {
             String::Utf8Value str(it->label->ToString());
             elm_object_item_text_set(it->li, *str);
          }
        if ( it->icon->IsObject())
          {
             it->icon_left = make_or_get(this, it->icon);
             if (it->icon_left)
               {
                  elm_icon_scale_set(it->icon_left->get(), 0, 0);
                  evas_object_size_hint_align_set(it->icon_left->get(), 0.0, 0.0);
                  elm_object_item_part_content_set(it->li, "left", it->icon_left->get());
               }
          }
        if ( it->end->IsObject())
          {
             it->icon_right = make_or_get(this, it->end);

             if (it->icon_right)
               {
                  elm_icon_scale_set(it->icon_right->get(), 0, 0);
                  evas_object_size_hint_align_set(it->icon_right->get(), 0.0, 0.0);
                  elm_object_item_part_content_set(it->li, "right", it->icon_right->get());
               }
          }
        if (it->tooltip->IsString())
          {
             String::Utf8Value str(it->tooltip->ToString());
             elm_object_tooltip_text_set(elm_object_item_widget_get(it->li), *str);
          }

        if (item->ToObject()->Get(String::New("on_clicked"))->IsFunction())
          {
             it->on_clicked = Local<Value>::New(
                       item->ToObject()->Get(String::New("on_clicked")));
          }
        elm_list_go(eo);
        return it;
     }

   virtual Handle<Value> mode_get() const
     {
        int mode = elm_list_mode_get(eo);
        return Number::New(mode);
     }

   virtual void mode_set(Handle<Value> value)
     {
        if (value->IsNumber())
          {
             int mode = value->NumberValue();
             elm_list_mode_set(eo, (Elm_List_Mode)mode);
          }
     }
};

template<> CEvasObject::CPropHandler<CElmList>::property_list
CEvasObject::CPropHandler<CElmList>::list[] = {
  PROP_HANDLER(CElmList, mode),
  { NULL, NULL, NULL },
};

class CElmEntry : public CEvasObject {
   FACTORY(CElmEntry)
protected:
   CPropHandler<CElmEntry> prop_handler;
public:
   CElmEntry(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_entry_add(parent->get());
        construct(eo, obj);
     }

   virtual Handle<Value> password_get() const
     {
        return Boolean::New(elm_entry_password_get(eo));
     }

   virtual void password_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_entry_password_set(eo, value->BooleanValue());
     }
   virtual Handle<Value> editable_get() const
     {
        return Boolean::New(elm_entry_editable_get(eo));
     }

   virtual void editable_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_entry_editable_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> line_wrap_get() const
     {
        return Integer::New(elm_entry_line_wrap_get(eo));
     }

   virtual void line_wrap_set(Handle<Value> value)
     {
        if (value->IsNumber())
          elm_entry_line_wrap_set(eo, (Elm_Wrap_Type)value->Int32Value());
     }

   virtual Handle<Value> scrollable_get() const
     {
        return Boolean::New(elm_entry_scrollable_get(eo));
     }

   virtual void scrollable_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_entry_scrollable_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> single_line_get() const
     {
        return Integer::New(elm_entry_single_line_get(eo));
     }

   virtual void single_line_set(Handle<Value> value)
     {
        if (value->IsNumber())
          elm_entry_single_line_set(eo, (Elm_Wrap_Type)value->Int32Value());
     }
};

template<> CEvasObject::CPropHandler<CElmEntry>::property_list
CEvasObject::CPropHandler<CElmEntry>::list[] = {
  PROP_HANDLER(CElmEntry, password),
  PROP_HANDLER(CElmEntry, editable),
  PROP_HANDLER(CElmEntry, line_wrap),
  PROP_HANDLER(CElmEntry, scrollable),
  PROP_HANDLER(CElmEntry, single_line),
  { NULL, NULL, NULL },
};

class CElmCheck : public CEvasObject {
   FACTORY(CElmCheck)
protected:
   CPropHandler<CElmCheck> prop_handler;
   Persistent<Value> the_icon;

public:
   CElmCheck(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_check_add(parent->get());
        construct(eo, obj);
     }

   virtual ~CElmCheck()
     {
        the_icon.Dispose();
     }

   virtual void state_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_check_state_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> state_get() const
     {
        return Boolean::New(elm_check_state_get(eo));
     }

   virtual Handle<Value> icon_get() const
     {
        return the_icon;
     }

   virtual void icon_set(Handle<Value> value)
     {
        the_icon.Dispose();
        CEvasObject *icon = make_or_get(this, value);
        elm_object_content_set(eo, icon->get());
        the_icon = Persistent<Value>::New(icon->get_object());
     }
};

template<> CEvasObject::CPropHandler<CElmCheck>::property_list
CEvasObject::CPropHandler<CElmCheck>::list[] = {
  PROP_HANDLER(CElmCheck, state),
  PROP_HANDLER(CElmCheck, icon),
  { NULL, NULL, NULL },
};

class CElmClock : public CEvasObject {
   FACTORY(CElmClock)
protected:
  CPropHandler<CElmClock> prop_handler;

public:
  CElmClock(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
    {
       eo = elm_clock_add(parent->top_widget_get());
       construct(eo, obj);
    }

  virtual ~CElmClock()
    {
    }

  virtual Handle<Value> show_am_pm_get() const
    {
       Eina_Bool show_am_pm = elm_clock_show_am_pm_get(eo);
       return Boolean::New(show_am_pm);
    }

  virtual void show_am_pm_set(Handle<Value> val)
    {
       if (val->IsNumber())
         {
            int value = val->ToNumber()->Value();
            elm_clock_show_am_pm_set(eo, value);
         }
    }

  virtual Handle<Value> show_seconds_get() const
    {
       Eina_Bool show_seconds = elm_clock_show_seconds_get(eo);
       return Boolean::New(show_seconds);
    }

  virtual void show_seconds_set(Handle<Value> val)
    {
       if (val->IsNumber())
         {
            int value = val->ToNumber()->Value();
            elm_clock_show_seconds_set(eo, value);
         }
    }

  virtual Handle<Value> hour_get() const
    {
       int hour = 0;
       elm_clock_time_get(eo, &hour, NULL, NULL);
       return Number::New(hour);
    }

  virtual Handle<Value> minute_get() const
    {
       int minute = 0;
       elm_clock_time_get(eo, NULL, &minute, NULL);
       return Number::New(minute);
    }

  virtual Handle<Value> second_get() const
    {
       int second = 0;
       elm_clock_time_get(eo, NULL, NULL, &second);
       return Number::New(second);
    }

  virtual void hour_set(Handle<Value> val)
    {
       if (!val->IsNumber())
         {
            ELM_ERR( "%s: value is not a Number!", __FUNCTION__);
            return;
         }
       int hour = 0;
       int minute = 0;
       int second = 0;
       // use either this or the class getters (involves conversion from Value to int)
       elm_clock_time_get(eo, &hour, &minute, &second);

       hour = val->ToNumber()->Value();
       elm_clock_time_set(eo, hour , minute, second);
    }

  virtual void minute_set(Handle<Value> val)
    {
       if (!val->IsNumber())
         {
            ELM_ERR( "%s: value is not a Number!", __FUNCTION__);
            return;
         }
       int hour = 0;
       int minute = 0;
       int second = 0;
       elm_clock_time_get(eo, &hour, &minute, &second);
       minute = val->ToNumber()->Value();
       elm_clock_time_set(eo, hour , minute, second);
    }

  virtual void second_set(Handle<Value> val)
    {
       if (!val->IsNumber())
         {
            ELM_ERR( "%s: value is not a Number!", __FUNCTION__);
            return;
         }
       int hour = 0;
       int minute = 0;
       int second = 0;
       elm_clock_time_get(eo, &hour, &minute, &second);
       second = val->ToNumber()->Value();
       elm_clock_time_set(eo, hour , minute, second);
    }

  virtual Handle<Value> edit_get() const
    {
       Eina_Bool editable = elm_clock_edit_get(eo);
       return Boolean::New(editable);
    }

  virtual void edit_set(Handle<Value> val)
    {
       if (val->IsBoolean())
         {
            Eina_Bool value = val->ToBoolean()->Value();
            elm_clock_edit_set(eo, value);
         }
    }
};

template<> CEvasObject::CPropHandler<CElmClock>::property_list
CEvasObject::CPropHandler<CElmClock>::list[] = {
  PROP_HANDLER(CElmClock, edit),
  PROP_HANDLER(CElmClock, hour),
  PROP_HANDLER(CElmClock, minute),
  PROP_HANDLER(CElmClock, second),
  PROP_HANDLER(CElmClock, show_seconds),
  PROP_HANDLER(CElmClock, show_am_pm),
  { NULL, NULL, NULL },
};

class CElmProgressBar : public CEvasObject {
   FACTORY(CElmProgressBar)
protected:
   CPropHandler<CElmProgressBar> prop_handler;
   Persistent<Value> the_icon;

   static Handle<Value> do_pulse(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmProgressBar *progress = static_cast<CElmProgressBar *>(self);
        if (args[0]->IsBoolean())
          progress->pulse(args[0]->BooleanValue());
        return Undefined();
     }

public:
   CElmProgressBar(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_progressbar_add(parent->get());
        construct(eo, obj);
        get_object()->Set(String::New("pulse"), FunctionTemplate::New(do_pulse)->GetFunction());
     }

   virtual ~CElmProgressBar()
     {
        the_icon.Dispose();
     }

   virtual void pulse(bool on)
     {
        elm_progressbar_pulse(eo, on);
     }

   virtual Handle<Value> icon_get() const
     {
        return the_icon;
     }

   virtual void icon_set(Handle<Value> value)
     {
        the_icon.Dispose();
        CEvasObject *icon = make_or_get(this, value);
        elm_object_content_set(eo, icon->get());
        the_icon = Persistent<Value>::New(icon->get_object());
     }

   virtual Handle<Value> inverted_get() const
     {
        return Boolean::New(elm_progressbar_inverted_get(eo));
     }

   virtual void inverted_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_progressbar_inverted_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> horizontal_get() const
     {
        return Boolean::New(elm_progressbar_horizontal_get(eo));
     }

   virtual void horizontal_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_progressbar_horizontal_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> units_get() const
     {
        return String::New(elm_progressbar_unit_format_get(eo));
     }

   virtual void units_set(Handle<Value> value)
     {
        if (value->IsString())
          {
             String::Utf8Value str(value);
             elm_progressbar_unit_format_set(eo, *str);
          }
     }

   virtual Handle<Value> span_get() const
     {
        return Integer::New(elm_progressbar_span_size_get(eo));
     }

   virtual void span_set(Handle<Value> value)
     {
        if (value->IsInt32())
          {
             int span = value->Int32Value();
             elm_progressbar_span_size_set(eo, span);
          }
     }

   virtual Handle<Value> pulser_get() const
     {
        return Boolean::New(elm_progressbar_pulse_get(eo));
     }

   virtual void pulser_set(Handle<Value> value)
     {
        if (value->IsBoolean())
          elm_progressbar_pulse_set(eo, value->BooleanValue());
     }

   virtual Handle<Value> value_get() const
     {
        return Number::New(elm_progressbar_value_get(eo));
     }

   virtual void value_set(Handle<Value> value)
     {
        if (value->IsNumber())
          elm_progressbar_value_set(eo, value->NumberValue());
     }
};

template<> CEvasObject::CPropHandler<CElmProgressBar>::property_list
CEvasObject::CPropHandler<CElmProgressBar>::list[] = {
  PROP_HANDLER(CElmProgressBar, icon),
  PROP_HANDLER(CElmProgressBar, inverted),
  PROP_HANDLER(CElmProgressBar, horizontal),
  PROP_HANDLER(CElmProgressBar, units),
  PROP_HANDLER(CElmProgressBar, span),
  PROP_HANDLER(CElmProgressBar, pulser),
  PROP_HANDLER(CElmProgressBar, value),
  { NULL, NULL, NULL },
};


class CElmPhoto : public CEvasObject {
   FACTORY(CElmPhoto)
protected:
  CPropHandler<CElmPhoto> prop_handler;

public:
  CElmPhoto(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
    {
       eo = elm_photo_add(parent->top_widget_get());
       construct(eo, obj);
    }

  virtual ~CElmPhoto()
    {
    }

  virtual Handle<Value> image_get() const
    {
       //No getter available
       return Undefined();
    }

  virtual void image_set(Handle<Value> val)
    {
       if (val->IsString())
         {
            String::Utf8Value str(val);

            if (0 > access(*str, R_OK))
              ELM_ERR( "warning: can't read image file %s", *str);

            Eina_Bool retval = elm_photo_file_set(eo, *str);
            if (retval == EINA_FALSE)
              ELM_ERR( "Unable to set the image");
         }
    }

  virtual Handle<Value> size_get() const
    {
       //No getter available
       return Undefined();
    }

  virtual void size_set(Handle<Value> val)
    {
       if (val->IsNumber())
         {
            int size = val->ToInt32()->Value();
            elm_photo_size_set(eo, size);
         }
    }

  virtual Handle<Value> fill_get() const
    {
       //No getter available
       return Undefined();
    }

  virtual void fill_set(Handle<Value> val)
    {
       if (val->IsBoolean())
         elm_photo_fill_inside_set(eo, val->BooleanValue());
    }
};

template<> CEvasObject::CPropHandler<CElmPhoto>::property_list
CEvasObject::CPropHandler<CElmPhoto>::list[] = {
  PROP_HANDLER(CElmPhoto, size),
  PROP_HANDLER(CElmPhoto, fill),
  { NULL, NULL, NULL },
};

class CElmSpinner : public CEvasObject {
   FACTORY(CElmSpinner)
protected:
  CPropHandler<CElmSpinner> prop_handler;

public:
  CElmSpinner(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
    {
       eo = elm_spinner_add(parent->top_widget_get());
       construct(eo, obj);
    }

  virtual ~CElmSpinner()
    {
    }

  virtual Handle<Value> label_format_get() const
    {
       return String::New(elm_spinner_label_format_get(eo));
    }

  virtual void label_format_set(Handle<Value> val)
    {
       if (val->IsString())
         {
            String::Utf8Value str(val);

            elm_spinner_label_format_set(eo, *str);
         }
    }

  virtual Handle<Value> step_get() const
    {
       return Number::New(elm_spinner_step_get(eo));
    }

  virtual void step_set(Handle<Value> val)
    {
       if (val->IsNumber())
         {
            int size = val->ToNumber()->Value();
            elm_spinner_step_set(eo, size);
         }
    }

   virtual Handle<Value> min_get() const
     {
        double min, max;
        elm_spinner_min_max_get(eo, &min, &max);
        return Number::New(min);
     }

   virtual void min_set(Handle<Value> value)
     {
        if (value->IsNumber())
          {
             double min, max;
             elm_spinner_min_max_get(eo, &min, &max);
             min = value->NumberValue();
             elm_spinner_min_max_set(eo, min, max);
          }
     }

   virtual Handle<Value> max_get() const
     {
        double min, max;
        elm_spinner_min_max_get(eo, &min, &max);
        return Number::New(max);
     }

   virtual void max_set(Handle<Value> value)
     {
        if (value->IsNumber())
          {
             double min, max;
             elm_spinner_min_max_get(eo, &min, &max);
             max = value->NumberValue();
             elm_spinner_min_max_set(eo, min, max);
          }
     }
  virtual Handle<Value> style_get() const
    {
       return String::New(elm_object_style_get(eo));
    }

  virtual void style_set(Handle<Value> val)
    {
       if (val->IsString())
         {
            String::Utf8Value str(val);
            elm_object_style_set(eo, *str);
         }
    }

  virtual Handle<Value> editable_get() const
    {
       return Number::New(elm_spinner_editable_get(eo));
    }

  virtual void editable_set(Handle<Value> val)
    {
       if (val->IsBoolean())
         {
            elm_spinner_editable_set(eo, val->BooleanValue());
         }
    }

  virtual Handle<Value> disabled_get() const
    {
       return Boolean::New(elm_spinner_editable_get(eo));
    }

  virtual void disabled_set(Handle<Value> val)
    {
       if (val->IsBoolean())
         {
            elm_object_disabled_set(eo, val->BooleanValue());
         }
    }

  virtual Handle<Value> special_value_get() const
    {
       //No getter available
       return Undefined();
    }

  virtual void special_value_set(Handle<Value> val)
    {
       if (val->IsObject())
         {
             Local<Value> value = val->ToObject()->Get(String::New("value"));
             Local<Value> label = val->ToObject()->Get(String::New("label"));
             String::Utf8Value str(label);
             int int_value = value->ToInt32()->Value();
             elm_spinner_special_value_add(eo, int_value, *str);
         }
    }
};

template<> CEvasObject::CPropHandler<CElmSpinner>::property_list
CEvasObject::CPropHandler<CElmSpinner>::list[] = {
  PROP_HANDLER(CElmSpinner, label_format),
  PROP_HANDLER(CElmSpinner, step),
  PROP_HANDLER(CElmSpinner, min),
  PROP_HANDLER(CElmSpinner, max),
  PROP_HANDLER(CElmSpinner, style),
  PROP_HANDLER(CElmSpinner, disabled),
  PROP_HANDLER(CElmSpinner, editable),
  PROP_HANDLER(CElmSpinner, special_value),
  { NULL, NULL, NULL },
};

class CElmPane : public CEvasObject {
   FACTORY(CElmPane)
protected:
  CPropHandler<CElmPane> prop_handler;

public:
  CElmPane(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
    {
       eo = elm_panes_add(parent->top_widget_get());
       construct(eo, obj);
       CEvasObject *left, *right;
       left = make_or_get(this, obj->Get(String::New("content_left")));
       if (left)
         {
            elm_object_part_content_set(eo, "elm.swallow.left", left->get());
         }

       right = make_or_get(this, obj->Get(String::New("content_right")));
       if (right)
         {
            elm_object_part_content_set(eo, "elm.swallow.right", right->get());
         }
    }

  virtual ~CElmPane()
    {
    }

  virtual Handle<Value> horizontal_get() const
    {
       return Number::New(elm_panes_horizontal_get(eo));
    }

  virtual void horizontal_set(Handle<Value> val)
    {
       if (val->IsBoolean())
         {
            elm_panes_horizontal_set(eo, val->BooleanValue());
         }
    }

   virtual void on_press_set(Handle<Value> val)
     {
        on_clicked_set(val);
     }

   virtual Handle<Value> on_press_get(void) const
     {
        return on_clicked_val;
     }

};

template<> CEvasObject::CPropHandler<CElmPane>::property_list
CEvasObject::CPropHandler<CElmPane>::list[] = {
  PROP_HANDLER(CElmPane, horizontal),
  PROP_HANDLER(CElmPane, on_press),
  { NULL, NULL, NULL },
};

class CElmBubble : public CEvasObject {
   FACTORY(CElmBubble)
protected:
  CPropHandler<CElmBubble> prop_handler;

public:
  CElmBubble(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
    {
       eo = elm_bubble_add(parent->top_widget_get());
       construct(eo, obj);
       CEvasObject *content;
       content = make_or_get(this, obj->Get(String::New("content")));
       if ( content )
         {
            elm_object_content_set(eo, content->get());
         }
    }

  virtual ~CElmBubble()
    {
    }

  virtual Handle<Value> text_part_get() const
    {
       return Undefined();
    }

  virtual void text_part_set(Handle<Value> val)
    {
       if (!val->IsObject())
         {
            ELM_ERR( "%s: value is not an object!", __FUNCTION__);
            return;
         }
       Local<Object> obj = val->ToObject();
       Local<Value> it = obj->Get(String::New("item"));
       Local<Value> lbl = obj->Get(String::New("label"));
       String::Utf8Value item(it);
       String::Utf8Value label(lbl);
       elm_object_part_text_set(eo, *item,*label);
    }


  virtual Handle<Value> corner_get() const
    {
       return String::New(elm_bubble_corner_get(eo));
    }

  virtual void corner_set(Handle<Value> val)
    {
       if (val->IsString())
         {
            String::Utf8Value str(val);
            elm_bubble_corner_set(eo, *str);
         }
    }

};

template<> CEvasObject::CPropHandler<CElmBubble>::property_list
CEvasObject::CPropHandler<CElmBubble>::list[] = {
  PROP_HANDLER(CElmBubble, text_part),
  PROP_HANDLER(CElmBubble, corner),
  { NULL, NULL, NULL },
};

class CElmSegment : public CEvasObject {
   FACTORY(CElmSegment)
protected:
  CPropHandler<CElmSegment> prop_handler;

public:
   CElmSegment(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_segment_control_add(parent->get());
        construct(eo, obj);
        //items_set(obj->Get(String::New("items")));
     }

   Handle<Object> items_set(Handle<Value> val)
     {
        /* add an list of children */
        Local<Object> out = Object::New();

        if (!val->IsObject())
          {
             ELM_ERR( "not an object!");
             return out;
          }

        Local<Object> in = val->ToObject();
        Local<Array> props = in->GetPropertyNames();

        /* iterate through elements and instantiate them */
        for (unsigned int i = 0; i < props->Length(); i++)
          {

             Local<Value> x = props->Get(Integer::New(i));
             String::Utf8Value val(x);

             Local<Value> item = in->Get(x->ToString());
             if (!item->IsObject())
               {
                  // FIXME: permit adding strings here?
                  ELM_ERR( "list item is not an object");
                  continue;
               }
             Local<Value> label = item->ToObject()->Get(String::New("label"));

             String::Utf8Value str(label);
             elm_segment_control_item_add(eo, NULL, *str);
          }

        return out;
     }

   virtual ~CElmSegment()
     {
     }

};

template<> CEvasObject::CPropHandler<CElmSegment>::property_list
CEvasObject::CPropHandler<CElmSegment>::list[] = {
  { NULL, NULL, NULL },
};

class CElmMenu : public CEvasObject {
   FACTORY(CElmMenu)
protected:
  CPropHandler<CElmMenu> prop_handler;

  class Item {
  public:
    Local<Value> on_clicked;
    Handle<Value> label;
    Handle<Value> icon;
    bool disabled;
  };

  class MenuItem : public Item {
  public:
    Elm_Object_Item *mi;
    MenuItem *next;
    MenuItem *prev;
    MenuItem *parent;
    MenuItem *child;
  };

  MenuItem *root;

public:
  CElmMenu(CEvasObject *par, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
    {
       eo = elm_menu_add(par->top_widget_get());
       root = NULL;
       construct(eo, obj);
       items_set(NULL, obj->Get(String::New("items")));
       get_object()->Set(String::New("addchild"), FunctionTemplate::New(addchild)->GetFunction());

       get_object()->Set(String::New("child"), FunctionTemplate::New(child)->GetFunction());
       get_object()->Set(String::New("parent"), FunctionTemplate::New(parent)->GetFunction());
       get_object()->Set(String::New("child_count"), FunctionTemplate::New(child_count)->GetFunction());
    }

  virtual ~CElmMenu()
    {
    }
   static Handle<Value> addchild(const Arguments&)
     {
        return Undefined();
     }

   static Handle<Value> parent(const Arguments&)
     {
        return Undefined();
     }

   static Handle<Value> child(const Arguments&)
     {
        return Undefined();
     }

   static Handle<Value> child_count(const Arguments&)
     {
        return Undefined();
     }

   static void eo_on_click(void *data, Evas_Object *, void *)
     {
       if (data)
         {
            Item *it = reinterpret_cast<Item *>(data);

            if (*it->on_clicked != NULL)
              {
                 if (it->on_clicked->IsFunction())
                   {
                      Handle<Function> fn(Function::Cast(*(it->on_clicked)));
                      fn->Call(fn, 0, NULL);
                   }
              }
         }
     }

   void items_set(MenuItem *parent, Handle<Value> val)
     {
       /* add a list of children */
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

            Local<Value> x = props->Get(Integer::New(i));
            String::Utf8Value val(x);

            Local<Value> item = in->Get(x->ToString());
            if (!item->IsObject())
              {
                 ELM_ERR( "list item is not an object");
                 continue;
              }

            MenuItem *par = new_item_set(parent, item);

            Local<Value> items_object = item->ToObject()->Get(String::New("items"));
            if (items_object->IsObject())
              {
                 items_set(par, items_object);
              }
         }
    }

   virtual MenuItem * new_item_set(MenuItem *parent, Handle<Value> item)
     {
        if (!item->IsObject())
          {
             // FIXME: permit adding strings here?
             ELM_ERR( "list item is not an object");
             return NULL;
          }
        Elm_Object_Item *par = NULL;
        if (parent!=NULL)
          {
             par = parent->mi;
          }

        Local<Value> sep_object = item->ToObject()->Get(String::New("seperator"));

        if ( sep_object->IsBoolean() )
          {
             // FIXME add if seperator : true, what if false
             if (sep_object->ToBoolean()->Value())
               {
                  elm_menu_item_separator_add(eo, par);
               }
             return parent;
          }
        else
          {
             MenuItem *it = NULL;

             it = new MenuItem();
             it->next = NULL;
             it->prev = NULL;
             it->child = NULL;
             it->parent = NULL;
             it->label = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("label")));
             it->icon = v8::Persistent<Value>::New(item->ToObject()->Get(String::New("icon")));
             it->on_clicked = Local<Value>::New(item->ToObject()->Get(String::New("on_clicked")));
             it->parent = parent;


             // either a label with icon
             if ( !it->label->IsString() && !it->icon->IsString() )
               {
                  ELM_ERR( "Not a label or seperator");
                  delete it;
                  return NULL;
               }

             String::Utf8Value label(it->label->ToString());
             String::Utf8Value icon(it->icon->ToString());

             Evas_Smart_Cb cb;
             void *data = NULL;

             if ( it->on_clicked->IsFunction() )
               {
                  cb = &eo_on_click;
                  data = reinterpret_cast<void *>(it);
               }

             it->mi = elm_menu_item_add(eo, par, *icon, *label, cb, data);

             //FIXME :: Refactor
             if (this->root==NULL)
               {
                  this->root = it;
               }
             else
               {
                  if (parent)
                    {
                       it->parent = parent;
                       if (parent->child==NULL)
                         {
                            parent->child = it;
                         }
                       else
                         {
                            MenuItem *ptr = parent->child;

                            while(ptr->next)
                              {
                                 ptr = ptr->next;
                              }

                            ptr->next = it;
                            it->prev = ptr;
                        }
                     }
                   else
                     {
                        MenuItem *ptr = this->root;
                        while(ptr->next)
                          {
                             ptr = ptr->next;
                          }
                        ptr->next = it;
                        it->prev = ptr;
                     }
               }

             Local<Value> disabled_object = item->ToObject()->Get(String::New("disabled"));

             if ( disabled_object->IsBoolean() )
               {
                  elm_object_item_disabled_set(it->mi, disabled_object->ToBoolean()->Value());
               }
             return it;
          }
     }

  virtual Handle<Value> move_get() const
    {
       return Undefined();
    }

  virtual void move_set(Handle<Value> val)
    {
        if (!val->IsObject())
          return;
        Local<Object> obj = val->ToObject();
        Local<Value> x = obj->Get(String::New("x"));
        Local<Value> y = obj->Get(String::New("y"));
        if (!x->IsNumber() || !y->IsNumber())
          return;
        Evas_Coord x_out = x->NumberValue();
        Evas_Coord y_out = y->NumberValue();
        elm_menu_move (eo, x_out, y_out);
    }
};

template<> CEvasObject::CPropHandler<CElmMenu>::property_list
CEvasObject::CPropHandler<CElmMenu>::list[] = {
  PROP_HANDLER(CElmMenu, move),
  { NULL, NULL, NULL },
};

class CElmColorSelector : public CEvasObject {
   FACTORY(CElmColorSelector)
protected:
   CPropHandler<CElmColorSelector> prop_handler;
   /* the on_clicked function */
   Persistent<Value> on_changed_val;

public:
  CElmColorSelector(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
    {
       eo = elm_colorselector_add(parent->top_widget_get());
       construct(eo, obj);
    }

  virtual ~CElmColorSelector()
    {
    }

  virtual Handle<Value> red_get() const
    {
       int r, g, b, a;
       elm_colorselector_color_get(eo, &r, &g, &b, &a);
       return Number::New(r);
    }

  virtual void red_set(Handle<Value> val)
    {
       if (val->IsNumber())
         {
            int r, g, b, a;
            elm_colorselector_color_get(eo, &r, &g, &b, &a);
            r = val->ToNumber()->Value();
            elm_colorselector_color_set(eo, r, g, b, a);
         }
    }

  virtual Handle<Value> green_get() const
    {
       int r, g, b, a;
       elm_colorselector_color_get(eo, &r, &g, &b, &a);
       return Number::New(g);
    }

  virtual void green_set(Handle<Value> val)
    {
       if (val->IsNumber())
         {
            int r, g, b, a;
            elm_colorselector_color_get(eo, &r, &g, &b, &a);
            g = val->ToNumber()->Value();
            elm_colorselector_color_set(eo, r, g, b, a);
         }
    }
  virtual Handle<Value> blue_get() const
    {
       int r, g, b, a;
       elm_colorselector_color_get(eo, &r, &g, &b, &a);
       return Number::New(b);
    }

  virtual void blue_set(Handle<Value> val)
    {
       if (val->IsNumber())
         {
            int r, g, b, a;
            elm_colorselector_color_get(eo, &r, &g, &b, &a);
            b = val->ToNumber()->Value();
            elm_colorselector_color_set(eo, r, g, b, a);
         }
    }
  virtual Handle<Value> alpha_get() const
    {
       int r, g, b, a;
       elm_colorselector_color_get(eo, &r, &g, &b, &a);
       return Number::New(a);
    }

  virtual void alpha_set(Handle<Value> val)
    {
       if (val->IsNumber())
         {
            int r, g, b, a;
            elm_colorselector_color_get(eo, &r, &g, &b, &a);
            a = val->ToNumber()->Value();
            elm_colorselector_color_set(eo, r, g, b, a);
         }
    }
   virtual void on_changed(void *)
     {
        Handle<Object> obj = get_object();
        HandleScope handle_scope;
        Handle<Value> val = on_changed_val;
        assert(val->IsFunction());
        Handle<Function> fn(Function::Cast(*val));
        Handle<Value> args[1] = { obj };
        fn->Call(obj, 1, args);
     }

   static void eo_on_changed(void *data, Evas_Object *, void *event_info)
     {
        CElmColorSelector *changed = static_cast<CElmColorSelector*>(data);
        changed->on_changed(event_info);
     }

   virtual void on_changed_set(Handle<Value> val)
     {
        on_changed_val.Dispose();
        on_changed_val = Persistent<Value>::New(val);
        if (val->IsFunction())
          evas_object_smart_callback_add(eo, "changed", &eo_on_changed, this);
        else
          evas_object_smart_callback_del(eo, "changed", &eo_on_changed);
     }

   virtual Handle<Value> on_changed_get(void) const
     {
        return on_changed_val;
     }

};

template<> CEvasObject::CPropHandler<CElmColorSelector>::property_list
CEvasObject::CPropHandler<CElmColorSelector>::list[] = {
  PROP_HANDLER(CElmColorSelector, red),
  PROP_HANDLER(CElmColorSelector, green),
  PROP_HANDLER(CElmColorSelector, blue),
  PROP_HANDLER(CElmColorSelector, alpha),
  PROP_HANDLER(CElmColorSelector, on_changed),
  { NULL, NULL, NULL },
};

class CElmCalendar : public CEvasObject {
   FACTORY(CElmCalendar)
protected:
   CPropHandler<CElmCalendar> prop_handler;
   /* the on_clicked function */
   Persistent<Value> on_changed_val;

public:
  CElmCalendar(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
    {
       eo = elm_calendar_add(parent->top_widget_get());
       construct(eo, obj);
       marks_set(obj->Get(String::New("marks")));
    }

  virtual ~CElmCalendar()
    {
    }

   Handle<Object> marks_set(Handle<Value> val)
     {
        /* add an list of children */
        Local<Object> out = Object::New();

        if (!val->IsObject())
          {
             ELM_ERR( "not an object!");
             return out;
          }

        Local<Object> in = val->ToObject();
        Local<Array> props = in->GetPropertyNames();
        struct tm mark_time;

        /* iterate through elements and instantiate them */
        for (unsigned int i = 0; i < props->Length(); i++)
          {

             Local<Value> x = props->Get(Integer::New(i));
             String::Utf8Value val(x);

             Local<Value> item = in->Get(x->ToString());
             if (!item->IsObject())
               {
                  String::Utf8Value xval(x->ToString());
                  ELM_ERR( "item is not an object %s", *xval);
                  continue;
               }

             Local<Value> type = item->ToObject()->Get(String::New("mark_type"));
             String::Utf8Value mark_type(type);
             Local<Value> date = item->ToObject()->Get(String::New("mark_date"));
             mark_time.tm_mday = date->ToNumber()->Value();
             Local<Value> mon = item->ToObject()->Get(String::New("mark_mon"));
             mark_time.tm_mon = mon->ToNumber()->Value();
             Local<Value> year = item->ToObject()->Get(String::New("mark_year"));
             mark_time.tm_year = year->ToNumber()->Value() - 1900;
             Local<Value> repeat = item->ToObject()->Get(String::New("mark_repeat"));
             String::Utf8Value mark_repeat(repeat);
             Elm_Calendar_Mark_Repeat intRepeat = ELM_CALENDAR_UNIQUE;

             if ( !strcasecmp(*mark_repeat, "annually"))
               intRepeat = ELM_CALENDAR_ANNUALLY;
             else if ( !strcasecmp(*mark_repeat, "monthly"))
               intRepeat = ELM_CALENDAR_MONTHLY;
             else if ( !strcasecmp(*mark_repeat, "weekly"))
               intRepeat = ELM_CALENDAR_WEEKLY;
             else if ( !strcasecmp(*mark_repeat, "daily"))
               intRepeat = ELM_CALENDAR_DAILY;

             elm_calendar_mark_add(eo, *mark_type, &mark_time, intRepeat);
             elm_calendar_marks_draw (eo);
          }

        return out;
     }

   virtual void on_changed(void *)
     {
        Handle<Object> obj = get_object();
        HandleScope handle_scope;
        Handle<Value> val = on_changed_val;
        assert(val->IsFunction());
        Handle<Function> fn(Function::Cast(*val));
        Handle<Value> args[1] = { obj };
        fn->Call(obj, 1, args);
     }

   static void eo_on_changed(void *data, Evas_Object *, void *event_info)
     {
        CElmCalendar *changed = static_cast<CElmCalendar*>(data);
        changed->on_changed(event_info);
     }

   virtual void on_changed_set(Handle<Value> val)
     {
        on_changed_val.Dispose();
        on_changed_val = Persistent<Value>::New(val);
        if (val->IsFunction())
          {
             evas_object_smart_callback_add(eo, "changed", &eo_on_changed, this);
          }
        else
          evas_object_smart_callback_del(eo, "changed", &eo_on_changed);
     }

   virtual Handle<Value> on_changed_get(void) const
     {
        return on_changed_val;
     }

   virtual Handle<Value> weekday_names_get(void) const
     {
        Local<Object> obj = Object::New();

        const char **wds = elm_calendar_weekdays_names_get(eo);
        obj->Set(String::New("0"), String::New(wds[0]));
        obj->Set(String::New("1"), String::New(wds[1]));
        obj->Set(String::New("2"), String::New(wds[2]));
        obj->Set(String::New("3"), String::New(wds[3]));
        obj->Set(String::New("4"), String::New(wds[4]));
        obj->Set(String::New("5"), String::New(wds[5]));
        obj->Set(String::New("6"), String::New(wds[6]));

        return obj;
     }

   virtual void weekday_names_set(Handle<Value> val)
     {
        if ( val->IsObject() )
          {
             const char *weekdays[7];
             Local<Object> obj = val->ToObject();

             for (int i = 0; i < 7 ; i++)
               {
                  char fill[2];
                  sprintf(fill, "%d", i);
                  Handle<Value> value = obj->Get(String::New(fill));
                  if ( value->IsString())
                    {
                       String::Utf8Value str(value);
                       weekdays[i] = strdup(*str);
                    }
               }
             elm_calendar_weekdays_names_set(eo,weekdays);
          }
     }

   virtual Handle<Value> min_year_get(void) const
     {
        int year_min, year_max;
        elm_calendar_min_max_year_get(eo, &year_min, &year_max);
        return Number::New(year_min);
     }

   virtual void min_year_set(Handle<Value> val)
     {
        if ( val->IsNumber() )
          {
             int year_min, year_max;
             elm_calendar_min_max_year_get(eo, &year_min, &year_max);
             year_min = val->ToNumber()->Value();
             elm_calendar_min_max_year_set(eo, year_min, year_max);
          }
     }

   virtual Handle<Value> max_year_get(void) const
     {
        int year_min, year_max;
        elm_calendar_min_max_year_get(eo, &year_min, &year_max);
        return Number::New(year_max);
     }

   virtual void max_year_set(Handle<Value> val)
     {
        if ( val->IsNumber() )
          {
             int year_min, year_max;
             elm_calendar_min_max_year_get(eo, &year_min, &year_max);
             year_max = val->ToNumber()->Value();
             elm_calendar_min_max_year_set(eo, year_min, year_max);
          }
     }

   virtual Handle<Value> day_selection_enabled_get(void) const
     {
        Eina_Bool day_select = elm_calendar_day_selection_enabled_get(eo);
        return Boolean::New(day_select);
     }

   virtual void day_selection_enabled_set(Handle<Value> val)
     {
        if ( val->IsBoolean() )
          {
             Eina_Bool day_select = val->ToBoolean()->Value();
             elm_calendar_day_selection_enabled_set(eo, day_select);
          }
     }

   virtual Handle<Value> selected_date_get(void) const
     {
        struct tm selected_time;
        elm_calendar_selected_time_get (eo,&selected_time);
        return Number::New(selected_time.tm_mday);
     }

   virtual void selected_date_set(Handle<Value> val)
     {
        if ( val->IsNumber() )
          {
             struct tm selected_time;
             elm_calendar_selected_time_get (eo,&selected_time);
             selected_time.tm_mday = val->ToNumber()->Value();
             elm_calendar_selected_time_set (eo,&selected_time);
          }
     }

   virtual Handle<Value> selected_month_get(void) const
     {
        struct tm selected_time;
        elm_calendar_selected_time_get (eo,&selected_time);
        return Number::New(selected_time.tm_mon);
     }

   virtual void selected_month_set(Handle<Value> val)
     {
        if ( val->IsNumber() )
          {
             struct tm selected_time;
             elm_calendar_selected_time_get (eo,&selected_time);
             selected_time.tm_mon = val->ToNumber()->Value();
             //tm_mon is zero based - but hide that from user.
             //let them give a normal number
             selected_time.tm_mon = selected_time.tm_mon - 1;
             elm_calendar_selected_time_set (eo,&selected_time);
          }
     }

   virtual Handle<Value> selected_year_get(void) const
     {
        struct tm selected_time;
        elm_calendar_selected_time_get (eo,&selected_time);
        return Number::New(selected_time.tm_year);
     }

   virtual void selected_year_set(Handle<Value> val)
     {
        if ( val->IsNumber() )
          {
             struct tm selected_time;
             elm_calendar_selected_time_get (eo,&selected_time);
             selected_time.tm_year = val->ToNumber()->Value();
             //tm_year is years since 1900 - but hide that from user.
             //let them give a normal year
             selected_time.tm_year = selected_time.tm_year - 1900;
             elm_calendar_selected_time_set (eo,&selected_time);
          }
     }

   virtual Handle<Value> calendar_interval_get(void) const
     {
        return Number::New(elm_calendar_interval_get(eo));
     }

   virtual void calendar_interval_set(Handle<Value> val)
     {
        if ( val->IsNumber() )
          {
             double interval = val->ToNumber()->Value();
             elm_calendar_interval_set(eo, interval);
          }
     }
};

template<> CEvasObject::CPropHandler<CElmCalendar>::property_list
CEvasObject::CPropHandler<CElmCalendar>::list[] = {
     PROP_HANDLER(CElmCalendar, weekday_names),
     PROP_HANDLER(CElmCalendar, min_year),
     PROP_HANDLER(CElmCalendar, max_year),
     PROP_HANDLER(CElmCalendar, day_selection_enabled),
     PROP_HANDLER(CElmCalendar, selected_date),
     PROP_HANDLER(CElmCalendar, selected_month),
     PROP_HANDLER(CElmCalendar, selected_year),
     PROP_HANDLER(CElmCalendar, calendar_interval),
     PROP_HANDLER(CElmCalendar, on_changed),
     { NULL, NULL, NULL },
};

class CElmTable : public CEvasObject {
   FACTORY(CElmTable)
protected:
   CPropHandler<CElmTable> prop_handler;
   std::list<CEvasObject *> table_items;

public:
   CElmTable(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_table_add(parent->top_widget_get());
        construct(eo, obj);
        get_object()->Set(String::New("unpack"), FunctionTemplate::New(unpack)->GetFunction());
        get_object()->Set(String::New("pack"), FunctionTemplate::New(pack)->GetFunction());
        get_object()->Set(String::New("clear"), FunctionTemplate::New(clear)->GetFunction());
        items_set(obj->Get(String::New("subobjects")));
     }

   static Handle<Value> pack(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmTable *table = static_cast<CElmTable *>(self);
        if (args[0]->IsObject())
          {
             return table->new_item_set(args[0]);
          }
        return Undefined();
     }

   static Handle<Value> unpack(const Arguments&)
     {
        return Undefined();
     }

   static Handle<Value> clear(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmTable *table = static_cast<CElmTable *>(self);
        elm_table_clear(table->get(), true);
        table->table_items.clear();
        return Undefined();
     }

   virtual void items_set(Handle<Value> val)
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
             Local<Value> x = props->Get(Integer::New(i));
             String::Utf8Value val(x);

             Local<Value> item = in->Get(x->ToString());

             new_item_set(item);
          }
     }

   virtual Handle<Value> new_item_set(Handle<Value> item)
     {
        CEvasObject *child = NULL;
        if (!item->IsObject())
          {
             // FIXME: permit adding strings here?
             ELM_ERR( "list item is not an object");
             return Undefined();
          }

        Local<Value> subobj = item->ToObject()->Get(String::New("subobject"));

        if ( subobj->IsObject())
          {
             child = make_or_get(this, subobj);
             if(!child)
                return Undefined();
          }
        else
          {
             return Undefined();
          }

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
             return Undefined();
          }

        int x,y,w,h;

        x = xpos->IntegerValue();
        y = ypos->IntegerValue();
        w = width->IntegerValue();
        h = height->IntegerValue();

        elm_table_pack(this->get(), child->get(), x, y, w, h);
        ELM_INF("Packing new table item at %d %d %d %d", x,y,w,h);
        table_items.push_back(child);
        return child->get_object();
     }

    void homogeneous_set(Handle<Value> val)
      {
         if (val->IsBoolean())
           elm_table_homogeneous_set(eo, val->BooleanValue());
      }

    virtual Handle<Value> homogeneous_get() const
      {
         return Boolean::New(elm_table_homogeneous_get(eo));
      }

    void padding_set(Handle<Value> val)
      {
         HandleScope handle_scope;
         if (!val->IsObject())
           return;
         Local<Object> obj = val->ToObject();
         Local<Value> x = obj->Get(String::New("x"));
         Local<Value> y = obj->Get(String::New("y"));
         if (!x->IsNumber() || !y->IsNumber())
           return;
         elm_table_padding_set(eo, x->NumberValue(), y->NumberValue());
      }

    virtual Handle<Value> padding_get() const
      {
         Evas_Coord x, y;
         elm_table_padding_get(eo, &x, &y);
         Local<Object> obj = Object::New();
         obj->Set(String::New("x"), Boolean::New(x));
         obj->Set(String::New("y"), Boolean::New(y));
         return obj;
      }
};

template<> CEvasObject::CPropHandler<CElmTable>::property_list
CEvasObject::CPropHandler<CElmTable>::list[] = {
  PROP_HANDLER(CElmTable, homogeneous),
  PROP_HANDLER(CElmTable, padding),
  { NULL, NULL, NULL },
};


class CElmPhotocam : public CEvasObject {
   FACTORY(CElmPhotocam)
protected:
   CPropHandler<CElmPhotocam> prop_handler;

public:
   CElmPhotocam(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_photocam_add(parent->top_widget_get());
        construct(eo, obj);
     }
   virtual void file_set(Handle<Value> val)
     {
       if (val->IsString())
         {
            String::Utf8Value str(val);
             if (0 > access(*str, R_OK))
               ELM_ERR( "warning: can't read image file %s", *str);
            elm_photocam_file_set(eo, *str);
            ELM_INF( "Photcam image file %s", *str);
         }
     }

   virtual Handle<Value> file_get(void) const
     {
        const char *f = NULL;
        f = elm_photocam_file_get (eo);
        if (f)
          return String::New(f);
        else
          return Null();
     }

   virtual Handle<Value> zoom_get() const
     {
        double zoom;
        zoom = elm_photocam_zoom_get(eo);
        return Number::New(zoom);
     }

   virtual void zoom_set(Handle<Value> value)
     {
        if (value->IsNumber())
          {
             double zoom;
             zoom = value->NumberValue();
             elm_photocam_zoom_set (eo, zoom);
          }
     }

   virtual Handle<Value> zoom_mode_get() const
     {
        Elm_Photocam_Zoom_Mode zoom_mode;
        zoom_mode = elm_photocam_zoom_mode_get(eo);
        return Number::New(zoom_mode);
     }

   virtual void zoom_mode_set(Handle<Value> value)
     {
        if (value->IsNumber())
          {
             Elm_Photocam_Zoom_Mode zoom_mode;
             zoom_mode = (Elm_Photocam_Zoom_Mode)value->NumberValue();
             elm_photocam_zoom_mode_set(eo, zoom_mode);
          }
     }

    virtual void bounce_set(Handle<Value> val)
     {
        bool x_bounce = false, y_bounce = false;
        if (get_xy_from_object(val, x_bounce, y_bounce))
          {
             elm_scroller_bounce_set(eo, x_bounce, y_bounce);
          }
     }

   virtual Handle<Value> bounce_get() const
     {
        Eina_Bool x, y;
        elm_scroller_bounce_get(eo, &x, &y);
        Local<Object> obj = Object::New();
        obj->Set(String::New("x"), Boolean::New(x));
        obj->Set(String::New("y"), Boolean::New(y));
        return obj;
     }

    void paused_set(Handle<Value> val)
      {
         if (val->IsBoolean())
           elm_photocam_paused_set(eo, val->BooleanValue());
      }

    virtual Handle<Value> paused_get() const
      {
         return Boolean::New(elm_photocam_paused_get(eo));
      }
};

template<> CEvasObject::CPropHandler<CElmPhotocam>::property_list
CEvasObject::CPropHandler<CElmPhotocam>::list[] = {
  PROP_HANDLER(CElmPhotocam, file),
  PROP_HANDLER(CElmPhotocam, zoom),
  PROP_HANDLER(CElmPhotocam, zoom_mode),
  PROP_HANDLER(CElmPhotocam, paused),
  PROP_HANDLER(CElmPhotocam, bounce),
  { NULL, NULL, NULL },
};


class CElmToggle : public CEvasObject {
   FACTORY(CElmToggle)
protected:
   CPropHandler<CElmToggle> prop_handler;

   /* the on_changed function */
   Persistent<Value> on_changed_val;
   Persistent<Value> the_icon;

public:
   CElmToggle(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_check_add(parent->top_widget_get());
        elm_object_style_set(eo, "toggle");
        construct(eo, obj);
     }

   static void eo_on_changed(void *data, Evas_Object *, void *event_info)
     {
        CElmToggle *changed = static_cast<CElmToggle*>(data);

        changed->on_changed(event_info);
     }

   virtual void on_changed(void *)
     {
        Handle<Object> obj = get_object();
        HandleScope handle_scope;
        Handle<Value> val = on_changed_val;
        // FIXME: pass event_info to the callback
        // FIXME: turn the pieces below into a do_callback method
        assert(val->IsFunction());
        Handle<Function> fn(Function::Cast(*val));
        Handle<Value> args[1] = { obj };
        fn->Call(obj, 1, args);
     }

   virtual void on_changed_set(Handle<Value> val)
     {
        on_changed_val.Dispose();
        on_changed_val = Persistent<Value>::New(val);
        if (val->IsFunction())
          evas_object_smart_callback_add(eo, "changed", &eo_on_changed, this);
        else
          evas_object_smart_callback_del(eo, "changed", &eo_on_changed);
     }

   virtual Handle<Value> on_changed_get(void) const
     {
        return on_changed_val;
     }


   virtual void onlabel_set(Handle<Value> val)
     {
       if (val->IsString())
         {
            String::Utf8Value str(val);
            elm_object_part_text_set(eo, "on", *str);
         }
     }

   virtual Handle<Value> onlabel_get(void) const
     {
        const char *onlabel = NULL;
        onlabel = elm_object_part_text_get(eo, "on");
        if (onlabel)
          return String::New(onlabel);
        else
          return Null();
     }

   virtual void offlabel_set(Handle<Value> val)
     {
       if (val->IsString())
         {
            String::Utf8Value str(val);
            elm_object_part_text_set(eo, "off", *str);
         }
     }

   virtual Handle<Value> offlabel_get(void) const
     {
        const char *offlabel = NULL;
        offlabel = elm_object_part_text_get(eo, "off");
        if (offlabel)
          return String::New(offlabel);
        else
          return Null();
     }

   virtual Handle<Value> icon_get() const
     {
        return the_icon;
     }

   virtual void icon_set(Handle<Value> value)
     {
        the_icon.Dispose();
        CEvasObject *icon = make_or_get(this, value);
        elm_object_content_set(eo, icon->get());
        the_icon = Persistent<Value>::New(icon->get_object());
     }

    void state_set(Handle<Value> val)
      {
         if (val->IsBoolean())
           elm_check_state_set(eo, val->BooleanValue());
      }

    virtual Handle<Value> state_get() const
      {
         return Boolean::New(elm_check_state_get(eo));
      }
};

template<> CEvasObject::CPropHandler<CElmToggle>::property_list
CEvasObject::CPropHandler<CElmToggle>::list[] = {
  PROP_HANDLER(CElmToggle, offlabel),
  PROP_HANDLER(CElmToggle, onlabel),
  PROP_HANDLER(CElmToggle, icon),
  PROP_HANDLER(CElmToggle, state),
  PROP_HANDLER(CElmToggle, on_changed),
  { NULL, NULL, NULL },
};


class CElmHover : public CEvasObject {
   FACTORY(CElmHover)
protected:
   CPropHandler<CElmHover> prop_handler;

   Persistent<Value> target;
   Persistent<Value> parent;

   /* the contents of Hover */
   Persistent<Value> top;
   Persistent<Value> top_left;
   Persistent<Value> top_right;
   Persistent<Value> bottom;
   Persistent<Value> bottom_left;
   Persistent<Value> bottom_right;
   Persistent<Value> left;
   Persistent<Value> right;
   Persistent<Value> middle;

public:
   CElmHover(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_hover_add(parent->top_widget_get());
        construct(eo, obj);
     }

   virtual void content_set(const char *swallow,Handle<Value> val)
     {
       if (val->IsObject())
         {

            CEvasObject *content = make_or_get(this,val);

            elm_object_part_content_set(eo, swallow, content->get());

            if (!strcmp(swallow, "top"))
              {
                 top.Dispose();
                 top = Persistent<Value>::New(content->get_object());
              }
            if (!strcmp(swallow, "top_left"))
              {
                 top_left.Dispose();
                 top_left = Persistent<Value>::New(content->get_object());
              }
            if (!strcmp(swallow, "top_right"))
              {
                 top_right.Dispose();
                 top_right = Persistent<Value>::New(content->get_object());
              }
            if (!strcmp(swallow, "bottom"))
              {
                 bottom.Dispose();
                 bottom = Persistent<Value>::New(content->get_object());
              }
            if (!strcmp(swallow, "bottom_left"))
              {
                 bottom_left.Dispose();
                 bottom_left = Persistent<Value>::New(content->get_object());
              }
            if (!strcmp(swallow, "bottom_right"))
              {
                 bottom_right.Dispose();
                 bottom_right = Persistent<Value>::New(content->get_object());
              }
            if (!strcmp(swallow, "left"))
              {
                 left.Dispose();
                 left = Persistent<Value>::New(content->get_object());
              }
            if (!strcmp(swallow, "right"))
              {
                 right.Dispose();
                 right = Persistent<Value>::New(content->get_object());
              }
            if (!strcmp(swallow, "middle"))
              {
                 middle.Dispose();
                 middle = Persistent<Value>::New(content->get_object());
              }
         }
     }

   virtual Handle<Value> content_get(const char *swallow) const
     {
        if (!strcmp(swallow, "top"))
          return top;
        if (!strcmp(swallow, "top_left"))
          return top_left;
        if (!strcmp(swallow, "top_right"))
          return top_right;
        if (!strcmp(swallow, "bottom"))
          return bottom;
        if (!strcmp(swallow, "bottom_left"))
          return bottom_left;
        if (!strcmp(swallow, "bottom_right"))
          return bottom_right;
        if (!strcmp(swallow, "left"))
          return left;
        if (!strcmp(swallow, "right"))
          return right;
        if (!strcmp(swallow, "middle"))
          return middle;

        return Null();
     }

   void top_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("top", val);
     }

   virtual Handle<Value> top_get() const
     {
        return content_get("top");
     }

   void top_left_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("top_left", val);
     }

   virtual Handle<Value> top_left_get() const
     {
        return content_get("top_left");
     }

   void top_right_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("top_right", val);
     }

   virtual Handle<Value> top_right_get() const
     {
        return content_get("top_right");
     }

   void bottom_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("bottom", val);
     }

   virtual Handle<Value> bottom_get() const
     {
        return content_get("bottom");
     }

   void bottom_left_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("bottom_left", val);
     }

   virtual Handle<Value> bottom_left_get() const
     {
        return content_get("bottom_left");
     }

   void bottom_right_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("bottom_right", val);
     }

   virtual Handle<Value> bottom_right_get() const
     {
        return content_get("bottom_right");
     }

   void left_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("left", val);
     }

   virtual Handle<Value> left_get() const
     {
        return content_get("left");
     }

   void right_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("right", val);
     }

   virtual Handle<Value> right_get() const
     {
        return content_get("right");
     }

   void middle_set(Handle<Value> val)
     {
        if (val->IsObject())
          content_set("middle", val);
     }

   virtual Handle<Value> middle_get() const
     {
        return content_get("middle");
     }
};

template<> CEvasObject::CPropHandler<CElmHover>::property_list
CEvasObject::CPropHandler<CElmHover>::list[] = {
  PROP_HANDLER(CElmHover, top),
  PROP_HANDLER(CElmHover, top_left),
  PROP_HANDLER(CElmHover, top_right),
  PROP_HANDLER(CElmHover, bottom),
  PROP_HANDLER(CElmHover, bottom_left),
  PROP_HANDLER(CElmHover, bottom_right),
  PROP_HANDLER(CElmHover, left),
  PROP_HANDLER(CElmHover, right),
  PROP_HANDLER(CElmHover, middle),
  { NULL, NULL, NULL },
};


class CElmFileSelectorButton : public CEvasObject {
   FACTORY(CElmFileSelectorButton)
protected:
   CPropHandler<CElmFileSelectorButton> prop_handler;

public:
   CElmFileSelectorButton(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_fileselector_button_add (parent->top_widget_get());
        construct(eo, obj);
     }

   virtual Handle<Value> win_title_get() const
     {
       return String::New(elm_fileselector_button_window_title_get(eo));

     }

   virtual void win_title_set(Handle<Value> val)
     {
        if (val->IsString() || val->IsNumber())
          {
             String::Utf8Value str(val);
             elm_fileselector_button_window_title_set(eo, *str);
          }
     }

   virtual Handle<Value> path_get() const
     {
       return String::New(elm_fileselector_button_path_get(eo));

     }

   virtual void path_set(Handle<Value> val)
     {
        if (val->IsString() || val->IsNumber())
          {
             String::Utf8Value str(val);
             elm_fileselector_button_path_set(eo, *str);
          }
     }
   virtual void win_size_set(Handle<Value> val)
     {
        if (!val->IsObject())
          return ;
        Evas_Coord width, height;
        Local<Object> obj = val->ToObject();
        Local<Value> w = obj->Get(String::New("w"));
        Local<Value> h = obj->Get(String::New("h"));
        if (!w->IsInt32() || !h->IsInt32())
          return;
        width = w->Int32Value();
        height = h->Int32Value();
        elm_fileselector_button_window_size_set (eo,  width, height);
     }

   virtual Handle<Value> win_size_get(void) const
     {
        Evas_Coord w, h;
        elm_fileselector_button_window_size_get  (eo,  &w, &h);
        Local<Object> obj = Object::New();
        obj->Set(String::New("w"), Number::New(w));
        obj->Set(String::New("h"), Number::New(h));
        return obj;
     }

   virtual Handle<Value> expandable_get() const
     {
        return Boolean::New(elm_fileselector_button_expandable_get(eo));
     }

   virtual void expandable_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_fileselector_button_expandable_set(eo, val->BooleanValue());
     }
   virtual Handle<Value> folder_only_get() const
     {
        return Boolean::New(elm_fileselector_button_folder_only_get(eo));
     }

   virtual void folder_only_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_fileselector_button_folder_only_set(eo, val->BooleanValue());
     }
   virtual Handle<Value> is_save_get() const
     {
        return Boolean::New(elm_fileselector_button_is_save_get (eo));
     }

   virtual void is_save_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_fileselector_button_is_save_set(eo, val->BooleanValue());
     }

   virtual Handle<Value> inwin_mode_get() const
     {
        return Boolean::New(elm_fileselector_button_inwin_mode_get(eo));
     }

   virtual void inwin_mode_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_fileselector_button_inwin_mode_set(eo, val->BooleanValue());
     }

   virtual void on_click(void *event_info)
     {
        Handle<Object> obj = get_object();
        HandleScope handle_scope;
        Handle<Value> val = on_clicked_val;

        // also provide x and y positions where it was clicked
        //
        if (event_info!=NULL)
          {
             Handle<String> path = String::New((const char *)event_info);
             Handle<Value> args[2] = { obj, path };
             assert(val->IsFunction());
             Handle<Function> fn(Function::Cast(*val));
             fn->Call(obj, 2, args);
          }
     }
   virtual void on_clicked_set(Handle<Value> val)
     {
        on_clicked_val.Dispose();
        on_clicked_val = Persistent<Value>::New(val);
        if (val->IsFunction())
          evas_object_smart_callback_add(eo, "file,chosen", &eo_on_click, this);
        else
          evas_object_smart_callback_del(eo, "file,chosen", &eo_on_click);
     }

   virtual Handle<Value> on_clicked_get(void) const
     {
        return on_clicked_val;
     }
};

template<> CEvasObject::CPropHandler<CElmFileSelectorButton>::property_list
CEvasObject::CPropHandler<CElmFileSelectorButton>::list[] = {
  PROP_HANDLER(CElmFileSelectorButton, win_title),
  PROP_HANDLER(CElmFileSelectorButton, win_size),
  PROP_HANDLER(CElmFileSelectorButton, path),
  PROP_HANDLER(CElmFileSelectorButton, expandable),
  PROP_HANDLER(CElmFileSelectorButton, folder_only),
  PROP_HANDLER(CElmFileSelectorButton, is_save),
  PROP_HANDLER(CElmFileSelectorButton, inwin_mode),
  { NULL, NULL, NULL },
};


class CElmFileSelectorEntry : public CEvasObject {
   FACTORY(CElmFileSelectorEntry)
protected:
   CPropHandler<CElmFileSelectorEntry> prop_handler;

public:
   CElmFileSelectorEntry(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_fileselector_entry_add (parent->top_widget_get());
        construct(eo, obj);
     }

   virtual Handle<Value> win_title_get() const
     {
       return String::New(elm_fileselector_entry_window_title_get(eo));

     }

   virtual void win_title_set(Handle<Value> val)
     {
        if (val->IsString() || val->IsNumber())
          {
             String::Utf8Value str(val);
             elm_fileselector_entry_window_title_set(eo, *str);
          }
     }

   virtual Handle<Value> selected_get() const
     {
       return String::New(elm_fileselector_entry_selected_get(eo));

     }

   virtual void selected_set(Handle<Value> val)
     {
        if (val->IsString() || val->IsNumber())
          {
             String::Utf8Value str(val);
             elm_fileselector_entry_selected_set(eo, *str);
          }
     }

   virtual Handle<Value> path_get() const
     {
       return String::New(elm_fileselector_entry_path_get(eo));

     }

   virtual void path_set(Handle<Value> val)
     {
        if (val->IsString() || val->IsNumber())
          {
             String::Utf8Value str(val);
             elm_fileselector_entry_path_set(eo, *str);
          }
     }
   virtual void win_size_set(Handle<Value> val)
     {
        if (!val->IsObject())
          return ;
        Evas_Coord width, height;
        Local<Object> obj = val->ToObject();
        Local<Value> w = obj->Get(String::New("w"));
        Local<Value> h = obj->Get(String::New("h"));
        if (!w->IsInt32() || !h->IsInt32())
          return;
        width = w->Int32Value();
        height = h->Int32Value();
        elm_fileselector_entry_window_size_set (eo,  width, height);
     }

   virtual Handle<Value> win_size_get(void) const
     {
        Evas_Coord w, h;
        elm_fileselector_entry_window_size_get  (eo,  &w, &h);
        Local<Object> obj = Object::New();
        obj->Set(String::New("w"), Number::New(w));
        obj->Set(String::New("h"), Number::New(h));
        return obj;
     }

   virtual Handle<Value> expandable_get() const
     {
        return Boolean::New(elm_fileselector_entry_expandable_get(eo));
     }

   virtual void expandable_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_fileselector_entry_expandable_set(eo, val->BooleanValue());
     }
   virtual Handle<Value> folder_only_get() const
     {
        return Boolean::New(elm_fileselector_entry_folder_only_get(eo));
     }

   virtual void folder_only_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_fileselector_entry_folder_only_set(eo, val->BooleanValue());
     }
   virtual Handle<Value> is_save_get() const
     {
        return Boolean::New(elm_fileselector_entry_is_save_get (eo));
     }

   virtual void is_save_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_fileselector_entry_is_save_set(eo, val->BooleanValue());
     }

   virtual Handle<Value> inwin_mode_get() const
     {
        return Boolean::New(elm_fileselector_entry_inwin_mode_get(eo));
     }

   virtual void inwin_mode_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_fileselector_entry_inwin_mode_set(eo, val->BooleanValue());
     }

   //TODO : Add support for more events.
   //"changed" 
   //"activated" 
   //"press" 
   //"longpressed" 
   //"clicked" 
   //"clicked,double" 
   //"focused" 
   //"unfocused" 
   //"selection,paste" 
   //"selection,copy" 
   //"selection,cut" 
   //"unpressed" 

   virtual void on_click(void *event_info)
     {
        Handle<Object> obj = get_object();
        HandleScope handle_scope;
        Handle<Value> val = on_clicked_val;

        // also provide x and y positions where it was clicked
        //
        if (event_info!=NULL)
          {
             Handle<String> path = String::New((const char *)event_info);
             Handle<Value> args[2] = { obj, path };
             assert(val->IsFunction());
             Handle<Function> fn(Function::Cast(*val));
             elm_fileselector_entry_path_set(eo, (const char *)event_info);
             fn->Call(obj, 2, args);
          }
     }
   virtual void on_clicked_set(Handle<Value> val)
     {
        on_clicked_val.Dispose();
        on_clicked_val = Persistent<Value>::New(val);
        if (val->IsFunction())
          evas_object_smart_callback_add(eo, "file,chosen", &eo_on_click, this);
        else
          evas_object_smart_callback_del(eo, "file,chosen", &eo_on_click);
     }

   virtual Handle<Value> on_clicked_get(void) const
     {
        return on_clicked_val;
     }
};

template<> CEvasObject::CPropHandler<CElmFileSelectorEntry>::property_list
CEvasObject::CPropHandler<CElmFileSelectorEntry>::list[] = {
  PROP_HANDLER(CElmFileSelectorEntry, win_title),
  PROP_HANDLER(CElmFileSelectorEntry, win_size),
  PROP_HANDLER(CElmFileSelectorEntry, path),
  PROP_HANDLER(CElmFileSelectorEntry, expandable),
  PROP_HANDLER(CElmFileSelectorEntry, folder_only),
  PROP_HANDLER(CElmFileSelectorEntry, is_save),
  PROP_HANDLER(CElmFileSelectorEntry, inwin_mode),
  PROP_HANDLER(CElmFileSelectorEntry, selected),
  { NULL, NULL, NULL },
};

class CElmInwin : public CEvasObject {
   FACTORY(CElmInwin)
protected:
   CPropHandler<CElmInwin> prop_handler;
   CEvasObject *content;

public:
   CElmInwin(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
          eo = elm_win_inwin_add(parent->top_widget_get());
          construct(eo, obj);
          content = make_or_get(this, obj->Get(String::New("content")));
          if (content)
            {
               elm_win_inwin_content_set(eo, content->get());
            }
     }

   virtual Handle<Value> activate_get() const
     {
        return Null();
     }

   virtual void activate_set(Handle<Value> val)
     {
        ELM_INF("Actiavted.");
        if (val->IsBoolean())
          elm_win_inwin_activate(eo);
     }

};

template<> CEvasObject::CPropHandler<CElmInwin>::property_list
CEvasObject::CPropHandler<CElmInwin>::list[] = {
  PROP_HANDLER(CElmInwin, activate),
  { NULL, NULL, NULL },
};

class CElmNotify : public CEvasObject {
   FACTORY(CElmNotify)
protected:
   CPropHandler<CElmNotify> prop_handler;
   CEvasObject *content;

public:
   CElmNotify(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_notify_add(parent->top_widget_get());
        construct(eo, obj);
     }

   virtual Handle<Value> content_get() const
     {
        return Undefined();
     }

   virtual void content_set(Handle<Value> val)
     {
        if (val->IsObject())
          {
             content = make_or_get(this, val);
             if (content)
               {
                  elm_object_content_set(eo, content->get());
               }
          }
     }

   virtual Handle<Value> orient_get() const
     {
        return Number::New(elm_notify_orient_get(eo));
     }

   virtual void orient_set(Handle<Value> val)
     {
        if (val->IsNumber())
          {
             double orient = val->ToInt32()->Value();
             elm_notify_orient_set(eo, (Elm_Notify_Orient)orient);
             ELM_INF("Value of orient = %g", orient);
          }
     }

   virtual Handle<Value> timeout_get() const
     {
        return Number::New(elm_notify_timeout_get(eo));
     }

   virtual void timeout_set(Handle<Value> val)
     {
        if (val->IsNumber())
          {
             double timeout = val->ToInt32()->Value();
             elm_notify_timeout_set(eo, timeout);
          }
     }

   virtual Handle<Value> repeat_events_get() const
     {
        return Boolean::New(elm_notify_repeat_events_get(eo));
     }

   virtual void repeat_events_set(Handle<Value> val)
     {
        if (val->IsBoolean())
          elm_notify_repeat_events_set(eo, val->BooleanValue());
     }

};

template<> CEvasObject::CPropHandler<CElmNotify>::property_list
CEvasObject::CPropHandler<CElmNotify>::list[] = {
  PROP_HANDLER(CElmNotify, content),
  PROP_HANDLER(CElmNotify, orient),
  PROP_HANDLER(CElmNotify, timeout),
  PROP_HANDLER(CElmNotify, repeat_events),
  { NULL, NULL, NULL },
};

#if 0

class CElmPager : public CEvasObject {
   FACTORY(CElmPager)
protected:
   std::list<CEvasObject *> pages;
   CPropHandler<CElmPager> prop_handler;

public:
   CElmPager(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_pager_add(parent->top_widget_get());
        construct(eo, obj);
        get_object()->Set(String::New("pop"), FunctionTemplate::New(pop)->GetFunction());
        get_object()->Set(String::New("push"), FunctionTemplate::New(push)->GetFunction());
        get_object()->Set(String::New("promote"), FunctionTemplate::New(promote)->GetFunction());
     }

   static Handle<Value> pop(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmPager *pager = static_cast<CElmPager *>(self);
        CEvasObject *content = pager->pages.front();

        if (content)
          {
             elm_pager_content_pop(pager->get());
             pager->pages.pop_front();
          }

        return Undefined();
     }

   static Handle<Value> push(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmPager *pager = static_cast<CElmPager *>(self);
        if (args[0]->IsObject())
          {
             CEvasObject *content = make_or_get(pager, args[0]);
             if (content)
               {
                  elm_pager_content_push(pager->get(), content->get());
                  pager->pages.push_front(content);
               }
          }
        return Undefined();
     }
   static Handle<Value> promote(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmPager *pager = static_cast<CElmPager *>(self);
        if (args[0]->IsObject())
          {
             CEvasObject *promotee = eo_from_info(args[0]->ToObject());

             if (promotee)
               elm_pager_content_promote(pager->get(), promotee->get());

          }
        return Undefined();
     }
};

template<> CEvasObject::CPropHandler<CElmPager>::property_list
CEvasObject::CPropHandler<CElmPager>::list[] = {
  { NULL, NULL, NULL },
};

#endif

class CElmNaviframe : public CEvasObject {
   FACTORY(CElmNaviframe)
protected:
   CPropHandler<CElmNaviframe> prop_handler;

public:
   CElmNaviframe(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_naviframe_add(parent->top_widget_get());
        construct(eo, obj);
        get_object()->Set(String::New("pop"), FunctionTemplate::New(pop)->GetFunction());
        get_object()->Set(String::New("push"), FunctionTemplate::New(push)->GetFunction());
     }

   static Handle<Value> pop(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmNaviframe *naviFrame = static_cast<CElmNaviframe *>(self);

        if (elm_naviframe_top_item_get(naviFrame->get()))
           elm_naviframe_item_pop(naviFrame->get());

        return Undefined();
     }

   static Handle<Value> push(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmNaviframe *naviFrame = static_cast<CElmNaviframe *>(self);
        CEvasObject *prev_btn = NULL, *next_btn = NULL, *content;
        bool has_prev_btn = false, has_next_btn = false;

        if (!args[0]->IsObject())
           return ThrowException(Exception::Error(String::New("Parameter 1 should be an object description or an elm.widget")));

        if (!args[1]->IsString())
           return ThrowException(Exception::Error(String::New("Parameter 2 should be a string")));

        if (args.Length() >= 3) {
           if (!args[2]->IsObject())
              return ThrowException(Exception::Error(String::New("Parameter 3 should either be undefined or an object description")));
           has_prev_btn = true;
        }
          
        if (args.Length() >= 4) {
           if (!args[3]->IsObject())
              return ThrowException(Exception::Error(String::New("Parameter 4 should either be undefined or an object description")));
           has_next_btn = true;
        }

        content = make_or_get(naviFrame, args[0]);
        if (!content)
           return ThrowException(Exception::Error(String::New("Could not create content from description")));

        if (has_prev_btn)
          {
             prev_btn = make_or_get(naviFrame, args[2]->ToObject());
             if (!prev_btn)
               return ThrowException(Exception::Error(String::New("Could not create back button from description")));
          }
        if (has_next_btn)
          {
             next_btn = make_or_get(naviFrame, args[3]->ToObject());
             if (!next_btn)
               return ThrowException(Exception::Error(String::New("Could not create next button from description")));
          }

        String::Utf8Value titleParam(args[1]->ToString());
        elm_naviframe_item_push(naviFrame->get(),
                                *titleParam,
                                has_prev_btn ? prev_btn->get() : 0,
                                has_next_btn ? next_btn->get() : 0,
                                content->get(),
                                0);
        return Undefined();
     }
};

template<> CEvasObject::CPropHandler<CElmNaviframe>::property_list
CEvasObject::CPropHandler<CElmNaviframe>::list[] = {
  { NULL, NULL, NULL },
};

class CElmGrid : public CEvasObject {
   FACTORY(CElmGrid)
protected:
   CPropHandler<CElmGrid> prop_handler;
   std::list<CEvasObject *> grid_items;

public:
   CElmGrid(CEvasObject *parent, Local<Object> obj) :
       CEvasObject(),
       prop_handler(property_list_base)
     {
        eo = elm_grid_add(parent->top_widget_get());
        construct(eo, obj);
        get_object()->Set(String::New("add"), FunctionTemplate::New(add)->GetFunction());
        get_object()->Set(String::New("clear"), FunctionTemplate::New(clear)->GetFunction());
        items_set(obj->Get(String::New("subobjects")));
     }

   static Handle<Value> add(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmGrid *grid = static_cast<CElmGrid *>(self);
        if (args[0]->IsObject())
          {
             grid->pack_set(args[0]);
          }
        return Undefined();
     }

   static Handle<Value> clear(const Arguments& args)
     {
        CEvasObject *self = eo_from_info(args.This());
        CElmGrid *grid = static_cast<CElmGrid *>(self);
        elm_grid_clear(grid->get(), true);
        grid->grid_items.clear();
        return Undefined();
     }

   virtual void items_set(Handle<Value> val)
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
             Local<Value> x = props->Get(Integer::New(i));
             String::Utf8Value val(x);
             Local<Value> item = in->Get(x->ToString());
             pack_set(item);
          }
     }

    void pack_set(Handle<Value> item)
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
         if(!child)
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

         int x,y,w,h;
         x = xpos->IntegerValue();
         y = ypos->IntegerValue();
         w = width->IntegerValue();
         h = height->IntegerValue();

         ELM_INF("Objects = %d %d %d %d", x,y,w,h);
         elm_grid_pack (this->get(), child->get(), x, y, w, h);
         grid_items.push_back(child);
       }

     virtual Handle<Value> pack_get() const
       {
         return Undefined();
       }

     void size_set(Handle<Value> val)
       {
          int x, y;
          if (get_xy_from_object(val, x, y))
            {
               ELM_INF("Grid Size = %d %d", x,y);
               elm_grid_size_set(eo, x, y);
            }
       }

     virtual Handle<Value> size_get() const
       {
          int x, y;
          elm_grid_size_get (eo, &x, &y);
          Local<Object> obj = Object::New();
          obj->Set(String::New("x"), Number::New(x));
          obj->Set(String::New("y"), Number::New(y));
          return obj;
       }
};

template<> CEvasObject::CPropHandler<CElmGrid>::property_list
CEvasObject::CPropHandler<CElmGrid>::list[] = {
  PROP_HANDLER(CElmGrid, size),
  PROP_HANDLER(CElmGrid, pack),
  { NULL, NULL, NULL },
};

static CEvasObject *
_make(CEvasObject *parent, Local<Object> description)
{
   String::Utf8Value widget_type(description->Get(String::New("type")));
   CEvasObject *eo = CEvasObject::make(*widget_type, parent, description);

   if (!eo)
     ELM_ERR("Unknown object type: \"%s\"", *widget_type);

   return eo;
}

static CEvasObject *
_get_evas_object(Local<Object> obj)
{
   return static_cast<CEvasObject*>(External::Unwrap(obj->Get(String::New("_eo"))));
}

CEvasObject *
make_or_get(CEvasObject *parent, Handle<Value> object_val)
{
   if (!object_val->IsObject())
     {
        ELM_ERR("%s: value is not an object!", __FUNCTION__);
        return NULL;
     }

   Local<Object> obj = object_val->ToObject();
   return obj->HasOwnProperty(String::New("_eo")) ? _get_evas_object(obj) : _make(parent, obj);
}

CElmBasicWindow *main_win;
Persistent<Value> the_datadir;
Persistent<Value> the_tmpdir;
Persistent<Value> the_theme;

Handle<Value>
elm_widget(const Arguments& args)
{
   if (args.Length() != 1)
     return ThrowException(Exception::Error(String::New("Bad parameters")));

   if (!args[0]->IsObject())
     return Undefined();

   Local<Value> parent = args[0]->ToObject()->Get(String::New("parent"));
   if (parent.IsEmpty())
     return ThrowException(Exception::Error(String::New("Parent not set")));
   
   CEvasObject *parentObject = _get_evas_object(parent->ToObject());
   if (!parentObject)
     return ThrowException(Exception::Error(String::New("Parent is not a widget")));

   CEvasObject *object = make_or_get(parentObject, args[0]->ToObject());
   if (!object)
     return ThrowException(Exception::Error(String::New("Could not realize widget")));

   return object->get_object();
}

Handle<Value>
elm_main_window(const Arguments& args)
{
   Local<String> win_name;
   Local<Number> win_type;

   if (args.Length() != 1)
     return ThrowException(Exception::Error(String::New("Bad parameters")));

   if (!args[0]->IsObject())
     return Undefined();

   if (!args[1]->IsString())
     win_name = String::New("main");

   if (!args[2]->IsNumber())
     win_type = Number::New(ELM_WIN_BASIC);

   main_win = new CElmBasicWindow(NULL, args[0]->ToObject(),
                                         win_name, //win name/class
                                         win_type); //win type
   if (!main_win)
     return Undefined();

   /*Elm_Theme *theme = elm_theme_new();
   char *envtheme = getenv("ELM_THEME");
   elm_theme_set(theme, envtheme);
   elm_object_theme_set(main_win->get(), theme);*/

   return main_win->get_object();
}

Handle<Value>
elm_loop_time(const Arguments&)
{
   return Number::New(ecore_loop_time_get());
}

Handle<Value>
elm_exit(const Arguments&)
{
   elm_exit();
   return Undefined();
}

Handle<Value>
datadir_getter(Local<String>, const AccessorInfo&)
{
   return the_datadir;
}

void
datadir_setter(Local<String>, Local<Value> value, const AccessorInfo&)
{
   the_datadir.Dispose();
   the_datadir = Persistent<Value>::New(value);
}

Handle<Value>
tmpdir_getter(Local<String>, const AccessorInfo&)
{
   return the_tmpdir;
}

void
tmpdir_setter(Local<String>, Local<Value> value, const AccessorInfo&)
{
   the_tmpdir.Dispose();
   the_tmpdir = Persistent<Value>::New(value);
}

Handle<Value>
theme_getter(Local<String>, const AccessorInfo&)
{
   return the_theme;
}

void
theme_setter(Local<String>, Local<Value> value, const AccessorInfo&)
{
   the_theme.Dispose();
   setenv("ELM_THEME",  *String::Utf8Value(value->ToString()), 1);

   the_theme = Persistent<Value>::New(value);
}

extern "C"
void RegisterModule(Handle<Object> target)
{
   int argc = 0;
   char *argv[] = {};

   elev8_elm_log_domain = eina_log_domain_register("elev8-elm", EINA_COLOR_GREEN);
   if (!elev8_elm_log_domain)
     {
        ELM_ERR( "could not register elev8-elm log domain.");
        elev8_elm_log_domain = EINA_LOG_DOMAIN_GLOBAL;
     }
   ELM_INF("elev8-elm Logging initialized. %d", elev8_elm_log_domain);

   elm_init(argc, argv);

   target->Set(String::NewSymbol("window"), FunctionTemplate::New(elm_main_window)->GetFunction());
   target->Set(String::NewSymbol("loop_time"), FunctionTemplate::New(elm_loop_time)->GetFunction());
   target->Set(String::NewSymbol("exit"), FunctionTemplate::New(elm_exit)->GetFunction());
   target->Set(String::NewSymbol("widget"), FunctionTemplate::New(elm_widget)->GetFunction());
   target->SetAccessor(String::NewSymbol("datadir"), datadir_getter, datadir_setter);
   target->SetAccessor(String::NewSymbol("tmpdir"), tmpdir_getter, tmpdir_setter);
   target->SetAccessor(String::NewSymbol("theme"), theme_getter, theme_setter);

   /* setup data directory */
   the_datadir = Persistent<String>::New(String::New(PACKAGE_DATA_DIR "/" ));
   the_tmpdir = Persistent<String>::New(String::New(PACKAGE_TMP_DIR "/" ));

   /* register widget types */
   CEvasObject::init_factory();

#define REGISTER(name_,type_) CEvasObject::register_widget(name_, type_::make)

   REGISTER("actionslider", CElmActionSlider);
   REGISTER("button", CElmButton);
   REGISTER("layout", CElmLayout);
   REGISTER("background", CElmBackground);
   REGISTER("check", CElmCheck);
   REGISTER("clock", CElmClock);
   REGISTER("entry", CElmEntry);
   REGISTER("flip", CElmFlip);
   REGISTER("list", CElmList);
   REGISTER("genlist", CElmGenList);
   REGISTER("icon", CElmIcon);
   REGISTER("label", CElmLabel);
   REGISTER("radio", CElmRadio);
   REGISTER("box", CElmBox);
   REGISTER("progressbar", CElmProgressBar);
   REGISTER("scroller", CElmScroller);
   REGISTER("segment", CElmSegment);
   REGISTER("image", CEvasImage);
   REGISTER("slider", CElmSlider);
   REGISTER("photo", CElmPhoto);
   REGISTER("spinner", CElmSpinner);
   REGISTER("pane", CElmPane);
   REGISTER("bubble", CElmBubble);
   REGISTER("menu", CElmMenu);
   REGISTER("colorselector", CElmColorSelector);
   REGISTER("calendar", CElmCalendar);
   REGISTER("table", CElmTable);
   REGISTER("photocam", CElmPhotocam);
   REGISTER("toggle", CElmToggle);
   REGISTER("fileselectorbutton", CElmFileSelectorButton);
   REGISTER("fileselectorentry", CElmFileSelectorEntry);
   REGISTER("inwin", CElmInwin);
   REGISTER("notify", CElmNotify);
#if 0
   REGISTER("pager", CElmPager);
#endif
   REGISTER("naviframe", CElmNaviframe);
   REGISTER("grid", CElmGrid);

#undef REGISTER
}
