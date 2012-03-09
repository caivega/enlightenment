#include "CElmCalendar.h"

CElmCalendar::CElmCalendar(CEvasObject *parent, Local<Object> obj) :
   CEvasObject(),
   prop_handler(property_list_base)
{
   eo = elm_calendar_add(parent->top_widget_get());
   construct(eo, obj);
   marks_set(obj->Get(String::New("marks")));
}

Handle<Object> CElmCalendar::marks_set(Handle<Value> val)
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

void CElmCalendar::on_changed(void *)
{
   Handle<Object> obj = get_object();
   HandleScope handle_scope;
   Handle<Value> val = on_changed_val;
   assert(val->IsFunction());
   Handle<Function> fn(Function::Cast(*val));
   Handle<Value> args[1] = { obj };
   fn->Call(obj, 1, args);
}

void CElmCalendar::eo_on_changed(void *data, Evas_Object *, void *event_info)
{
   CElmCalendar *changed = static_cast<CElmCalendar*>(data);
   changed->on_changed(event_info);
}

void CElmCalendar::on_changed_set(Handle<Value> val)
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

Handle<Value> CElmCalendar::on_changed_get(void) const
{
   return on_changed_val;
}

Handle<Value> CElmCalendar::weekday_names_get(void) const
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

void CElmCalendar::weekday_names_set(Handle<Value> val)
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

Handle<Value> CElmCalendar::min_year_get(void) const
{
   int year_min, year_max;
   elm_calendar_min_max_year_get(eo, &year_min, &year_max);
   return Number::New(year_min);
}

void CElmCalendar::min_year_set(Handle<Value> val)
{
   if ( val->IsNumber() )
     {
        int year_min, year_max;
        elm_calendar_min_max_year_get(eo, &year_min, &year_max);
        year_min = val->ToNumber()->Value();
        elm_calendar_min_max_year_set(eo, year_min, year_max);
     }
}

Handle<Value> CElmCalendar::max_year_get(void) const
{
   int year_min, year_max;
   elm_calendar_min_max_year_get(eo, &year_min, &year_max);
   return Number::New(year_max);
}

void CElmCalendar::max_year_set(Handle<Value> val)
{
   if ( val->IsNumber() )
     {
        int year_min, year_max;
        elm_calendar_min_max_year_get(eo, &year_min, &year_max);
        year_max = val->ToNumber()->Value();
        elm_calendar_min_max_year_set(eo, year_min, year_max);
     }
}

Handle<Value> CElmCalendar::day_selection_enabled_get(void) const
{
   Eina_Bool day_select = elm_calendar_day_selection_enabled_get(eo);
   return Boolean::New(day_select);
}

void CElmCalendar::day_selection_enabled_set(Handle<Value> val)
{
   if ( val->IsBoolean() )
     {
        Eina_Bool day_select = val->ToBoolean()->Value();
        elm_calendar_day_selection_enabled_set(eo, day_select);
     }
}

Handle<Value> CElmCalendar::selected_date_get(void) const
{
   struct tm selected_time;
   elm_calendar_selected_time_get (eo,&selected_time);
   return Number::New(selected_time.tm_mday);
}

void CElmCalendar::selected_date_set(Handle<Value> val)
{
   if ( val->IsNumber() )
     {
        struct tm selected_time;
        elm_calendar_selected_time_get (eo,&selected_time);
        selected_time.tm_mday = val->ToNumber()->Value();
        elm_calendar_selected_time_set (eo,&selected_time);
     }
}

Handle<Value> CElmCalendar::selected_month_get(void) const
{
   struct tm selected_time;
   elm_calendar_selected_time_get (eo,&selected_time);
   return Number::New(selected_time.tm_mon);
}

void CElmCalendar::selected_month_set(Handle<Value> val)
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

Handle<Value> CElmCalendar::selected_year_get(void) const
{
   struct tm selected_time;
   elm_calendar_selected_time_get (eo,&selected_time);
   return Number::New(selected_time.tm_year);
}

void CElmCalendar::selected_year_set(Handle<Value> val)
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

Handle<Value> CElmCalendar::calendar_interval_get(void) const
{
   return Number::New(elm_calendar_interval_get(eo));
}

void CElmCalendar::calendar_interval_set(Handle<Value> val)
{
   if ( val->IsNumber() )
     {
        double interval = val->ToNumber()->Value();
        elm_calendar_interval_set(eo, interval);
     }
}

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
