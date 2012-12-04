#ifndef EET_DATA_H
#define EET_DATA_H
#include "Clouseau.h"
#include <Ecore_Con_Eet.h>
/*  Global constants  */
#define BMP_FIELD "bmp"

#define PORT           (22522)
#define MAX_LINE       (1023)
#define LOCALHOST      "127.0.0.1"
#define LISTEN_IP      "0.0.0.0" /* Avail all, no mask */

#define CLOUSEAU_GUI_CLIENT_CONNECT_STR "CLOUSEAU_GUI_CLIENT_CONNECT"
#define CLOUSEAU_APP_CLIENT_CONNECT_STR "CLOUSEAU_APP_CLIENT_CONNECT"
#define CLOUSEAU_APP_ADD_STR "CLOUSEAU_APP_ADD"
#define CLOUSEAU_DATA_REQ_STR "CLOUSEAU_DATA_REQ"
#define CLOUSEAU_TREE_DATA_STR "CLOUSEAU_TREE_DATA"
#define CLOUSEAU_APP_CLOSED_STR "CLOUSEAU_APP_CLOSED"
#define CLOUSEAU_HIGHLIGHT_STR "CLOUSEAU_HIGHLIGHT"
#define CLOUSEAU_BMP_REQ_STR "CLOUSEAU_BMP_REQ"
#define CLOUSEAU_BMP_DATA_STR "CLOUSEAU_BMP_DATA"

/* Private function */
#define CLOUSEAU_APP_ADD_ENTRY   "clouseau/app"
#define CLOUSEAU_TREE_DATA_ENTRY "clouseau/app/tree"
#define CLOUSEAU_BMP_LIST_ENTRY  "clouseau/app/shot_list"
#define CLOUSEAU_BMP_DATA_ENTRY  "clouseau/app/screenshot"

struct _connect_st
{  /* This will be used for APP, GUI client connect */
   unsigned int pid;
   const char *name;
};
typedef struct _connect_st connect_st;

struct _app_info_st
{  /* This will be used to register new APP in GUI client */
   unsigned int pid;
   char *name;
   char *file;          /* Valid only if was read from file in offline mode */
   unsigned long long ptr; /* (void *) client ptr of app as saved by daemon */
   Eina_List *view;        /* Screen views pointers of (bmp_info_st *)      */
   unsigned int refresh_ctr;      /* Counter of how many times down refresh */
};
typedef struct _app_info_st app_info_st;

struct _data_req_st
{  /* This will be used to ask for tree data by DAEMON or GUI */
   unsigned long long gui; /* (void *) client ptr of GUI */
   unsigned long long app; /* (void *) client ptr APP    */
};
typedef struct _data_req_st data_req_st;

struct _tree_data_st
{  /* This will be used to send tree data to/from APP/DAEMON */
   unsigned long long gui; /* (void *) client ptr of GUI */
   unsigned long long app; /* (void *) client ptr APP    */
   Eina_List *tree; /* The actual (Tree_Item *) list */
};
typedef struct _tree_data_st tree_data_st;

struct _app_closed_st
{  /* This will be used to notify GUI of app closed   */
   unsigned long long ptr; /* (void *) client ptr APP */
};
typedef struct _app_closed_st app_closed_st;

struct _highlight_st
{  /* This will be used to highlight object in APP */
   unsigned long long app; /* (void *) client ptr of APP */
   unsigned long long object; /* (void *) object ptr of object to highlight */
};
typedef struct _highlight_st highlight_st;

struct _bmp_req_st
{  /* This will be used to send tree data to/from APP/DAEMON */
   unsigned long long gui; /* (void *) client ptr of GUI */
   unsigned long long app; /* (void *) client ptr APP    */
   unsigned long long object; /* (void *) object ptr of Evas */
   unsigned int ctr;       /* Reload counter to match    */
};
typedef struct _bmp_req_st bmp_req_st;

struct _bmp_info_st
{  /* This will be used to send app window Bitmap             */
   /* We are using ULONGLONG because we send this as RAW data */
   /* win, bt are NOT transferred.                            */
   unsigned long long gui;     /* (void *) client ptr of GUI  */
   unsigned long long app;     /* (void *) client ptr of APP  */
   unsigned long long object;  /* (void *) object ptr of evas */
   unsigned long long ctr;     /* Reload counter to match     */
   unsigned long long w;       /* BMP width, make  Evas_Coord */
   unsigned long long h;       /* BMP hight, make  Evas_Coord */

   /* All the following fields are NOT transferred in EET msg */
   Evas_Object *win;           /* Window of view if open      */
   Evas_Object *scr;           /* Scroller holds view         */
   Evas_Object *o;             /* Actuall object displays BMP */
   double zoom_val;            /* Current zoom value          */
   Evas_Object *lb_mouse;      /* Label contains mouse cords  */
   Evas_Object *lb_rgba;       /* Current mouse pos rgba val  */
   Evas_Object *bt;            /* Button opening win          */
   Evas_Object *lx;            /* Line on X axis              */
   Evas_Object *ly;            /* Line on Y axis              */
   void *bmp;      /* Bitmap BLOB, size (w * h * sizeof(int)) */
};
typedef struct _bmp_info_st bmp_info_st;

struct _shot_list_st
{  /* This will be used to write a shot list to eet file */
   Eina_List *view;       /* Screen views eahc is (bmp_info_st *) ptr */
};
typedef struct _shot_list_st shot_list_st;

struct _data_desc
{
   Eet_Data_Descriptor *bmp_data;
   Eet_Data_Descriptor *bmp_req;
   Eet_Data_Descriptor *bmp_info;
   Eet_Data_Descriptor *shot_list;
   Eet_Data_Descriptor *connect;
   Eet_Data_Descriptor *app_add;
   Eet_Data_Descriptor *data_req;
   Eet_Data_Descriptor *tree_data;
   Eet_Data_Descriptor *app_closed;
   Eet_Data_Descriptor *highlight;
   Eet_Data_Descriptor *tree;
   Eet_Data_Descriptor *obj_info;
};
typedef struct _data_desc data_desc;

/* Exported From Object information */
EAPI void clouseau_object_information_free(Clouseau_Object *oinfo);
EAPI Clouseau_Object * clouseau_object_information_get(Clouseau_Tree_Item *treeit);

/* Exported function */
EAPI void clouseau_data_tree_free(Eina_List *tree);
EAPI void *clouseau_data_packet_compose(const char *p_type, void *data, unsigned int *size, void *blob, int blob_size);
EAPI void *clouseau_data_packet_info_get(const char *p_type, void *data, size_t size);
EAPI void clouseau_data_object_highlight(Evas_Object *obj, Clouseau_Evas_Props *props, bmp_info_st *view);
EAPI Eina_Bool clouseau_data_eet_info_save(const char *filename, app_info_st *a, tree_data_st *ftd, Eina_List *ck_list);
EAPI Eina_Bool clouseau_data_eet_info_read(const char *filename, app_info_st **a, tree_data_st **ftd);
EAPI int clouseau_data_init(void);
EAPI int clouseau_register_descs(Ecore_Con_Eet *eet_svr);
EAPI int clouseau_data_shutdown(void);
#endif  /*  EET_DATA_H  */
