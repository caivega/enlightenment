#include <Elementary.h>
#include "elm_priv.h"

#define OBJ       "/org/freedesktop/Notifications"
#define BUS       "org.freedesktop.Notifications"
#define INTERFACE "org.freedesktop.Notifications"

#ifdef ELM_EDBUS2
static Eina_Bool _elm_need_sys_notify = EINA_FALSE;

static EDBus_Connection *_elm_sysnotif_conn  = NULL;
static EDBus_Object     *_elm_sysnotif_obj   = NULL;
static EDBus_Proxy      *_elm_sysnotif_proxy = NULL;

static Eina_Bool _has_markup = EINA_FALSE;

typedef struct _Elm_Sys_Notify_Send_Data
{
   Elm_Sys_Notify_Send_Cb cb;
   const void *data;
} Elm_Sys_Notify_Send_Data;

static void
_elm_sys_notify_marshal_dict_byte(EDBus_Message_Iter *array,
                                  const char *key,
                                  const char value)
{
   EDBus_Message_Iter *var, *entry;

   edbus_message_iter_arguments_set(array, "{sv}", &entry);
   edbus_message_iter_basic_append(entry, 's', key);

   var = edbus_message_iter_container_new(entry, 'v', "y");
   edbus_message_iter_basic_append(var, 'y', value);
   edbus_message_iter_container_close(entry, var);
   edbus_message_iter_container_close(array, entry);
}

static void
_elm_sys_notify_marshal_dict_string(EDBus_Message_Iter *array,
                                   const char *key,
                                   const char *value)
{
   EDBus_Message_Iter *var, *entry;

   edbus_message_iter_arguments_set(array, "{sv}", &entry);
   edbus_message_iter_basic_append(entry, 's', key);

   var = edbus_message_iter_container_new(entry, 'v', "s");
   edbus_message_iter_basic_append(var, 's', value);
   edbus_message_iter_container_close(entry, var);
   edbus_message_iter_container_close(array, entry);
}

static void
_get_capabilities_cb(void *data EINA_UNUSED,
                     const EDBus_Message *msg,
                     EDBus_Pending *pending EINA_UNUSED)
{
   char *val;
   EDBus_Message_Iter *arr;

   if (edbus_message_error_get(msg, NULL, NULL) ||
       !edbus_message_arguments_get(msg, "as", &arr)) goto end;

   while(edbus_message_iter_get_and_next(arr, 's', &val))
     if (!strcmp(val, "body-markup"))
       {
          _has_markup = EINA_TRUE;
          return;
       }

end:
   _has_markup = EINA_FALSE;
}

void
_elm_sys_notify_capabilities_get(void)
{
   EINA_SAFETY_ON_NULL_RETURN(_elm_sysnotif_proxy);

   if (!edbus_proxy_call(_elm_sysnotif_proxy, "GetCapabilities",
                         _get_capabilities_cb, NULL, -1, ""))
     ERR("Error sending message: "INTERFACE".GetCapabilities.");
}

static void
_close_notification_cb(void *data EINA_UNUSED,
                       const EDBus_Message *msg,
                       EDBus_Pending *pending EINA_UNUSED)
{
   const char *errname, *errmsg;

   if (edbus_message_error_get(msg, &errname, &errmsg))
     {
        if (errmsg && errmsg[0] == '\0')
          INF("Notification no longer exists.");
        else
          ERR("Edbus Error: %s %s", errname, errmsg);
     }
}
#endif

EAPI void
elm_sys_notify_close(unsigned int id)
{
#ifdef ELM_EDBUS2
   EINA_SAFETY_ON_FALSE_RETURN(_elm_need_sys_notify);
   EINA_SAFETY_ON_NULL_RETURN(_elm_sysnotif_proxy);

   if (!edbus_proxy_call(_elm_sysnotif_proxy, "CloseNotification",
                         _close_notification_cb, NULL, -1, "u", id))
     ERR("Error sending message: "INTERFACE".CloseNotification.");
#else
   (void) id;
#endif
}

#ifdef ELM_EDBUS2
static void
_notify_cb(void *data,
           const EDBus_Message *msg,
           EDBus_Pending *pending EINA_UNUSED)
{
   const char *errname, *errmsg;
   Elm_Sys_Notify_Send_Data *d = data;
   unsigned int id = 0;

   if (edbus_message_error_get(msg, &errname, &errmsg))
     ERR("Error: %s %s", errname, errmsg);
   else if (!edbus_message_arguments_get(msg, "u", &id))
     {
        ERR("Error getting return values of "INTERFACE".Notify.");
        id = 0;
     }

   if (d->cb) d->cb((void *)d->data, id);
   free(d);
}
#endif

EAPI void
elm_sys_notify_send(unsigned int replaces_id, const char *icon,
                    const char *summary, const char *body,
                    Elm_Sys_Notify_Urgency urgency, int timeout,
                    Elm_Sys_Notify_Send_Cb cb, const void *cb_data)
{
#ifdef ELM_EDBUS2
   EDBus_Message *msg;
   EDBus_Message_Iter *iter, *actions, *hints;
   Elm_Sys_Notify_Send_Data *data;
   char *body_free = NULL;
   char *desk_free = NULL;
   const char *deskentry = elm_app_desktop_entry_get();
   const char *appname = elm_app_name_get();

   EINA_SAFETY_ON_FALSE_RETURN(_elm_need_sys_notify);
   EINA_SAFETY_ON_NULL_RETURN(_elm_sysnotif_proxy);

   data = malloc(sizeof(Elm_Sys_Notify_Send_Data));
   EINA_SAFETY_ON_NULL_GOTO(data, error);
   data->cb = cb;
   data->data = cb_data;

   if (!icon) icon = "";
   if (!summary) summary = "";
   if (!body)
     body = "";
   else if (!_has_markup)
     body = body_free = elm_entry_markup_to_utf8(body);

   msg = edbus_proxy_method_call_new(_elm_sysnotif_proxy, "Notify");

   iter = edbus_message_iter_get(msg);
   edbus_message_iter_arguments_set(iter, "susssas", appname, replaces_id, icon,
                                    summary, body, &actions);
   /* actions */
   edbus_message_iter_container_close(iter, actions);

   /* hints */
   edbus_message_iter_arguments_set(iter, "a{sv}", &hints);
   _elm_sys_notify_marshal_dict_byte(hints, "urgency", (char) urgency);

   if (strcmp(deskentry, ""))
     {
        deskentry = ecore_file_file_get(deskentry);
        deskentry = desk_free = ecore_file_strip_ext(deskentry);
        _elm_sys_notify_marshal_dict_string(hints, "desktop_entry", deskentry);
     }
   edbus_message_iter_container_close(iter, hints);

   /* timeout */
   edbus_message_iter_arguments_set(iter, "i", timeout);

   edbus_proxy_send(_elm_sysnotif_proxy, msg, _notify_cb, data, -1);

   edbus_message_unref(msg);
   free(desk_free);
   free(body_free);
   return;

error:
#else
   (void) replaces_id;
   (void) icon;
   (void) summary;
   (void) body;
   (void) urgency;
   (void) timeout;
#endif
   if (cb) cb((void *)cb_data, 0);
}

#ifdef ELM_EDBUS2
static void
_release(void)
{
   if (_elm_sysnotif_proxy)
     {
        edbus_proxy_unref(_elm_sysnotif_proxy);
        _elm_sysnotif_proxy = NULL;
     }

   if (_elm_sysnotif_obj)
     {
        edbus_object_unref(_elm_sysnotif_obj);
        _elm_sysnotif_obj = NULL;
     }
}

static void
_update(void)
{
   _release();
   _elm_sysnotif_obj = edbus_object_get(_elm_sysnotif_conn, BUS, OBJ);
   _elm_sysnotif_proxy = edbus_proxy_get(_elm_sysnotif_obj, INTERFACE);
   _elm_sys_notify_capabilities_get();
}

static void
_name_owner_get_cb(void *data EINA_UNUSED,
                   const EDBus_Message *msg,
                   EDBus_Pending *pending EINA_UNUSED)
{
   const char *errname, *errmsg;

   if (edbus_message_error_get(msg, &errname, &errmsg))
     ERR("Edbus Error: %s %s", errname, errmsg);
   else
     _update();
}

static void
_name_owner_changed_cb(void *data EINA_UNUSED,
                       const char *bus EINA_UNUSED,
                       const char *old_id EINA_UNUSED,
                       const char *new_id)
{
   if ((!new_id) || (*new_id == '\0'))
     _release();
   else
     _update();
}
#endif

EAPI Eina_Bool
elm_need_sys_notify(void)
{
#ifdef ELM_EDBUS2
   if (_elm_need_sys_notify) return EINA_TRUE;

   if (!elm_need_edbus()) return EINA_FALSE;

   _elm_sysnotif_conn = edbus_connection_get(EDBUS_CONNECTION_TYPE_SESSION);

   edbus_name_owner_changed_callback_add(_elm_sysnotif_conn, BUS,
                                         _name_owner_changed_cb, NULL,
                                         EINA_FALSE);

   edbus_name_owner_get(_elm_sysnotif_conn, BUS, _name_owner_get_cb, NULL);

   _elm_need_sys_notify = EINA_TRUE;

   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif
}

void
_elm_unneed_sys_notify(void)
{
#ifdef ELM_EDBUS2
   if (!_elm_need_sys_notify) return;

   _elm_need_sys_notify = EINA_FALSE;

   _release();

   edbus_connection_unref(_elm_sysnotif_conn);
   _elm_sysnotif_conn  = NULL;
#endif
}
