#ifndef _TSUITE_EVAS_HOOK_H
#define _TSUITE_EVAS_HOOK_H

#include "tsuite.h"

#define CACHE_FILE_ENTRY "cache"

struct _Timer_Data
{
   Eet_File *fp;
   unsigned int recent_event_time;
   Evas *e;
   Eina_List *current_event;
};
typedef struct _Timer_Data Timer_Data;

/**
 * @internal
 *
 * @struct _Tsuite_Data
 * Struct holds test-suite data-properties
 *
 * @ingroup Tsuite
 */
struct _Tsuite_Data
{
   char *name;    /**< Test Name */
   int serial;    /**< Serial number of currnen-file */
   api_data *api;
   Evas *e;
   Evas_Object *win;
   Timer_Data *td;
};
typedef struct _Tsuite_Data Tsuite_Data;

struct _mouse_in_mouse_out
{
   unsigned int timestamp;
};

struct _mouse_down_mouse_up
{
   int b;
   Evas_Button_Flags flags;
   unsigned int timestamp;
};

struct _mouse_move
{
   int x;
   int y;
   unsigned int timestamp;
};

struct _mouse_wheel
{
   int direction;
   int z;
   unsigned int timestamp;
};

struct _key_down_key_up
{
   unsigned int timestamp;
   const char *keyname;
   const char *key;
   const char *string;
   const char *compose;
};

struct _multi_event
{
   int d;
   int x;
   int y;
   double rad;
   double radx;
   double rady;
   double pres;
   double ang;
   double fx;
   double fy;
   Evas_Button_Flags flags;
   unsigned int timestamp;
};

struct _multi_move
{
   int d;
   int x;
   int y;
   double rad;
   double radx;
   double rady;
   double pres;
   double ang;
   double fx;
   double fy;
   unsigned int timestamp;
};

typedef struct _mouse_in_mouse_out mouse_in_mouse_out;
typedef struct _mouse_down_mouse_up mouse_down_mouse_up;
typedef struct _mouse_move mouse_move;
typedef struct _mouse_wheel mouse_wheel;
typedef struct _multi_event multi_event;
typedef struct _multi_move multi_move;
typedef struct _key_down_key_up key_down_key_up;
typedef struct _mouse_in_mouse_out take_screenshot;

/* START - EET support typedefs */
#define TSUITE_EVENT_MOUSE_IN_STR "tsuite_event_mouse_in"
#define TSUITE_EVENT_MOUSE_OUT_STR "tsuite_event_mouse_out"
#define TSUITE_EVENT_MOUSE_DOWN_STR "tsuite_event_mouse_down"
#define TSUITE_EVENT_MOUSE_UP_STR "tsuite_event_mouse_up"
#define TSUITE_EVENT_MOUSE_MOVE_STR "tsuite_event_mouse_move"
#define TSUITE_EVENT_MOUSE_WHEEL_STR "tsuite_event_mouse_wheel"
#define TSUITE_EVENT_MULTI_DOWN_STR "tsuite_event_multi_down"
#define TSUITE_EVENT_MULTI_UP_STR "tsuite_event_multi_up"
#define TSUITE_EVENT_MULTI_MOVE_STR "tsuite_event_multi_move"
#define TSUITE_EVENT_KEY_DOWN_STR "tsuite_event_key_down"
#define TSUITE_EVENT_KEY_UP_STR "tsuite_event_key_up"
#define TSUITE_EVENT_TAKE_SHOT_STR "tsuite_event_take_shot"

struct _Lists_st
{
   Eina_List *variant_list;
};
typedef struct _Lists_st Lists_st;

struct _data_desc
{
   Eet_Data_Descriptor *take_screenshot;
   Eet_Data_Descriptor *mouse_in_mouse_out;
   Eet_Data_Descriptor *mouse_down_mouse_up;
   Eet_Data_Descriptor *mouse_move;
   Eet_Data_Descriptor *mouse_wheel;
   Eet_Data_Descriptor *multi_event;
   Eet_Data_Descriptor *multi_move;
   Eet_Data_Descriptor *key_down_key_up;

   /* list, variant EET desc support */
   Eet_Data_Descriptor *_lists_descriptor;
   Eet_Data_Descriptor *_variant_descriptor;
   Eet_Data_Descriptor *_variant_unified_descriptor;
};
typedef struct _data_desc data_desc;
/* END   - EET support typedefs */

/* START Event struct descriptors */
Eet_Data_Descriptor *take_screenshot_desc_make(void);
Eet_Data_Descriptor *mouse_in_mouse_out_desc_make(void);
Eet_Data_Descriptor *mouse_down_mouse_up_desc_make(void);
Eet_Data_Descriptor *mouse_move_desc_make(void);
Eet_Data_Descriptor *mouse_wheel_desc_make(void);
Eet_Data_Descriptor *key_down_key_up_desc_make(void);
Eet_Data_Descriptor *multi_event_desc_make(void);
Eet_Data_Descriptor *multi_move_desc_make(void);
data_desc *_data_descriptors_init(void);
void _data_descriptors_shutdown(void);
/* END   Event struct descriptors */

Tsuite_Event_Type tsuite_event_mapping_type_get(const char *name);
const char * tsuite_event_mapping_type_str_get(Tsuite_Event_Type t);
const char * _variant_type_get(const void *data, Eina_Bool *unknow);
Eina_Bool _variant_type_set(const char *type, void *data, Eina_Bool unknow);

Lists_st * free_events(Lists_st *st, char *recording);
void write_events(const char *filename, Lists_st *vr_list);
Lists_st *read_events(char *filename, Evas *e, Timer_Data *td);
#endif
