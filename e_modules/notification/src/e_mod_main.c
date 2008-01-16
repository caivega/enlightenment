#include <E_Notification_Daemon.h>
#include <e.h>

#undef __UNUSED__
#define __UNUSED__ __attribute__((unused))

typedef struct _Popup_Data Popup_Data;
struct _Popup_Data
{
  E_Notification *notif;
  E_Win *win;
  Evas *e;
  Evas_Object *theme;
  Evas_Object *app_icon;
  Ecore_Timer *timer;
};

typedef struct _Daemon_Data Daemon_Data;
struct _Daemon_Data
{
  E_Notification_Daemon *daemon;
  Evas_List *popups;
  
  float default_timeout;
  int next_id;
};

/* local function protos */
static int  _notification_cb_notify(E_Notification_Daemon *daemon, E_Notification *n);
static void _notification_cb_close_notification(E_Notification_Daemon *daemon, 
                                                unsigned int id);
static int  _notification_timer_cb(void *data);
static void _notification_theme_cb_deleted(void *data, Evas_Object *obj, 
                                           const char *emission, const char *source);

static Popup_Data *_notification_popup_new(E_Notification *n);
static void        _notification_popup_place(Popup_Data *popup);
static void        _notification_popup_refresh(Popup_Data *popup);
static Popup_Data *_notification_popup_find(unsigned int id);
static void        _notification_popup_del(unsigned int id, 
                                           E_Notification_Closed_Reason reason);
static void        _notification_popdown(Popup_Data *popup, 
                                         E_Notification_Closed_Reason reason);

static char *_notification_format_message(E_Notification *n);

/* Global variables */
static Daemon_Data *dd;
static E_Module    *notification_mod = NULL;

/* Module Api Functions */
EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Notification"};

EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Notification_Daemon *d;

   dd = calloc(1, sizeof(Daemon_Data));

   /* set up the daemon */
   d = e_notification_daemon_add("e_notification_module", "Enlightenment");
   e_notification_daemon_data_set(d, dd);
   dd->daemon = d;
   dd->default_timeout = 5.0;
   e_notification_daemon_callback_notify_set(d, _notification_cb_notify);
   e_notification_daemon_callback_close_notification_set(d, _notification_cb_close_notification);

   notification_mod = m;
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m __UNUSED__) 
{
   e_notification_daemon_free(dd->daemon);
   free(dd);
   notification_mod = NULL;
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__) 
{
   return 1;
}

/* Callbacks */
static int
_notification_cb_notify(E_Notification_Daemon *daemon __UNUSED__, E_Notification *n)
{
   unsigned int replaces_id;
   unsigned int new_id;
   int timeout;
   Popup_Data *popup = NULL;
   
   replaces_id = e_notification_replaces_id_get(n);
   if (replaces_id && (popup = _notification_popup_find(replaces_id))) 
     {
       if (popup->notif) e_notification_unref(popup->notif);
       e_notification_ref(n);
       popup->notif = n;
       edje_object_signal_emit(popup->theme, "notification,del", "notification");
     }

   if (!popup)
     {
       popup = _notification_popup_new(n);
       dd->popups = evas_list_append(dd->popups, popup);
       edje_object_signal_emit(popup->theme, "notification,new", "notification");
     }
   
   new_id = dd->next_id++;
   e_notification_id_set(n, new_id);
   
   if (popup->timer) ecore_timer_del(popup->timer);
   timeout = e_notification_timeout_get(popup->notif);
   popup->timer = ecore_timer_add(timeout == -1 ? dd->default_timeout : (float)timeout / 1000, 
                                  _notification_timer_cb, 
                                  popup);
   
   return new_id;
}

static void
_notification_cb_close_notification(E_Notification_Daemon *daemon __UNUSED__, 
                                    unsigned int id)
{
   _notification_popup_del(id, 
                           E_NOTIFICATION_CLOSED_REQUESTED);
}

static int
_notification_timer_cb(void *data)
{
   Popup_Data *popup = data;
   _notification_popup_del(e_notification_id_get(popup->notif), 
                           E_NOTIFICATION_CLOSED_EXPIRED);
   return 0;
}

static void
_notification_theme_cb_deleted(void *data, 
                               Evas_Object *obj __UNUSED__, 
                               const char *emission __UNUSED__, 
                               const char *source __UNUSED__)
{
  Popup_Data *popup = data;
  _notification_popup_refresh(popup);
  edje_object_signal_emit(popup->theme, "notification,new", "notification");
}

/* Local functions */
static Popup_Data *
_notification_popup_new(E_Notification *n)
{
   E_Container *con;
   Popup_Data *popup;
   char buf[PATH_MAX];
   Ecore_X_Window_State state[5] = { 
     ECORE_X_WINDOW_STATE_STICKY,
     ECORE_X_WINDOW_STATE_SKIP_TASKBAR,
     ECORE_X_WINDOW_STATE_SKIP_PAGER,
     ECORE_X_WINDOW_STATE_HIDDEN,
     ECORE_X_WINDOW_STATE_ABOVE
   };

   popup = calloc(1, sizeof(Popup_Data));
   if (!popup) return NULL;
   e_notification_ref(n);
   popup->notif = n;

   con = e_container_current_get(e_manager_current_get());

   /* Create the popup window */
   popup->win = e_win_new(con);
   e_win_name_class_set(popup->win, "E", "_notification_dialog");
   e_win_title_set(popup->win, "Event Notification");
   e_win_borderless_set(popup->win, 1);
   e_win_placed_set(popup->win, 1);
   e_win_sticky_set(popup->win, 1);
   ecore_x_icccm_transient_for_set(popup->win->evas_win, con->win);
   ecore_x_icccm_protocol_set(popup->win->evas_win, ECORE_X_WM_PROTOCOL_TAKE_FOCUS, 0);
   ecore_x_netwm_window_type_set(popup->win->evas_win, ECORE_X_WINDOW_TYPE_DOCK);
   ecore_x_netwm_window_state_set(popup->win->evas_win, state, 5);

   popup->e = e_win_evas_get(popup->win);

   /* Setup the theme */
   snprintf(buf, sizeof(buf), "%s/e-module-notification.edj", notification_mod->dir);
   popup->theme = edje_object_add(popup->e);
   if (!e_theme_edje_object_set(popup->theme, "base/theme/modules/notification",
                                "modules/notification/main"))
     edje_object_file_set(popup->theme, buf, "modules/notification/main");
   evas_object_show(popup->theme);
   edje_object_signal_callback_add(popup->theme, "notification,deleted", "theme", 
                                   _notification_theme_cb_deleted, popup);

   /* Uncomment to use shaped popups */
   //e_win_shaped_set(popup->win, 1);
   //e_win_avoid_damage_set(popup->win, 1);

   _notification_popup_refresh(popup);
   _notification_popup_place(popup);
   e_win_show(popup->win);

   return popup;
}

static void
_notification_popup_place(Popup_Data *popup)
{
   int h;
   evas_object_geometry_get(popup->theme, NULL, NULL, NULL, &h);
   e_win_move(popup->win, 10, 10 + evas_list_count(dd->popups) * (h + 10));
}

static void
_notification_popup_refresh(Popup_Data *popup)
{
   const char *icon_path;
   char *msg;
   void *img;
   int w, h;

   if (!popup) return;

   if (popup->app_icon) 
     {
       edje_object_part_unswallow(popup->theme, popup->app_icon);
       evas_object_del(popup->app_icon);
     }

   /* Check if the app specify an icon either by a path or by a hint */
   if ((icon_path = e_notification_app_icon_get(popup->notif)) && *icon_path)
     {
       popup->app_icon = evas_object_image_add(popup->e);
       evas_object_image_load_scale_down_set(popup->app_icon, 1);
       evas_object_image_load_size_set(popup->app_icon, 80, 80);
       evas_object_image_file_set(popup->app_icon, icon_path, NULL);
       evas_object_image_fill_set(popup->app_icon, 0, 0, 80, 80);
     }
   else if ((img = e_notification_hint_icon_data_get(popup->notif)))
     {
       popup->app_icon = e_notification_image_evas_object_add(popup->e, img);
     }
   evas_object_image_size_get(popup->app_icon, &w, &h);
   edje_extern_object_min_size_set(popup->app_icon, w, h);
   edje_extern_object_max_size_set(popup->app_icon, w, h);
   edje_object_part_swallow(popup->theme, "notification.swallow.app_icon", popup->app_icon);

   /* Fill up the event message */
   msg = _notification_format_message(popup->notif);
   edje_object_part_text_set(popup->theme, "notification.textblock.message", msg);
   free(msg);

   /* Compute the new size of the popup */
   edje_object_size_min_calc(popup->theme, &w, &h);
   e_win_size_min_set(popup->win, w, h);
   e_win_size_max_set(popup->win, w, h);
   e_win_resize(popup->win, w, h);
   evas_object_resize(popup->theme, w, h);
   edje_object_calc_force(popup->theme);
}

static Popup_Data *
_notification_popup_find(unsigned int id)
{
   Evas_List *l;
   Popup_Data *popup;

   for (l = dd->popups; l && (popup = l->data); l = l->next)
     if (e_notification_id_get(popup->notif) == id) return popup;

   return NULL;
}

static void
_notification_popup_del(unsigned int id, E_Notification_Closed_Reason reason)
{
   Popup_Data *popup;
   Evas_List *l, *next;
   int i;

   for (l = dd->popups, i = 0; l && (popup = l->data); l = next)
     {
       next = l->next;
       if (e_notification_id_get(popup->notif) == id)
         {
           _notification_popdown(popup, reason);
           dd->popups = evas_list_remove_list(dd->popups, l);
         }
       else
         {
           int h;
           evas_object_geometry_get(popup->theme, NULL, NULL, NULL, &h);
           e_win_move(popup->win, 10, 10 + i * (h + 10));
           i++;
         }
     }
}

static void
_notification_popdown(Popup_Data *popup, E_Notification_Closed_Reason reason)
{
   ecore_timer_del(popup->timer);
   e_win_hide(popup->win);
   evas_object_del(popup->app_icon);
   evas_object_del(popup->theme);
   e_object_del(E_OBJECT(popup->win));
   e_notification_closed_set(popup->notif, 1);
   e_notification_daemon_signal_notification_closed(dd->daemon, 
                                                    e_notification_id_get(popup->notif), 
                                                    reason);
   e_notification_unref(popup->notif);
   free(popup);
}

static char *
_notification_format_message(E_Notification *n)
{
   char *msg;
   const char *orig;
   char *dest;
   int len;
   int size = 512;

   msg = calloc(1, 512);
   snprintf(msg, 511, "<subject>%s</subject><br><body>", 
            e_notification_summary_get(n));
   len = strlen(msg);

   for (orig = e_notification_body_get(n), dest = msg + strlen(msg); orig && *orig; orig++)
     {
       if (len >= size - 4)
         {
           size = len + 512;
           msg = realloc(msg, size);
           msg = memset(msg + len, 0, size - len);
           dest = msg + len;
         }

       if (*orig == '\n')
         {
           dest[0] = '<'; 
           dest[1] = 'b'; 
           dest[2] = 'r'; 
           dest[3] = '>';
           len += 4;
           dest += 4;
         }
       else
         {
           *dest = *orig;
           len++;
           dest++;
         }
     }

   return msg;
}

