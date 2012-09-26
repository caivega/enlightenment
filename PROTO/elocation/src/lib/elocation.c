#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <Ecore.h>
#include <E_DBus.h>
#include <Elocation.h>
#include <elocation_private.h>

static char *unique_name = NULL;

static E_DBus_Signal_Handler *cb_name_owner_changed = NULL;
static DBusPendingCall *pending_get_name_owner = NULL;

static Elocation_Provider master_provider;

static Eina_Bool
geoclue_start(void *data, int ev_type, void *event)
{
   printf("GeoClue start event\n");
   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
geoclue_stop(void *data, int ev_type, void *event)
{
   printf("GeoClue stop event\n");
   return ECORE_CALLBACK_DONE;
}

static void
create_cb(void *data , DBusMessage *reply, DBusError *error)
{
   const char *object_path;
   DBusMessageIter iter;

   if (!dbus_message_has_signature(reply, "o")) return;

   dbus_message_iter_init(reply, &iter);
   dbus_message_iter_get_basic(&iter, &object_path);

   printf("Object path for client: %s\n", object_path);
}

static void
_system_name_owner_changed(void *data , DBusMessage *msg)
{
   DBusError err;
   const char *name, *from, *to;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_STRING, &name,
                              DBUS_TYPE_STRING, &from,
                              DBUS_TYPE_STRING, &to,
                              DBUS_TYPE_INVALID))
     {
        dbus_error_free(&err);
        return;
     }

   if (strcmp(name, GEOCLUE_DBUS_NAME) != 0)
      return;

   if (from[0] == '\0' && to[0] != '\0')
     {
        ecore_event_add(ELOCATION_EVENT_IN, NULL, NULL, NULL);
        unique_name = strdup(to);
     }
   else if (from[0] != '\0' && to[0] == '\0')
     {
        if (strcmp(unique_name, from) != 0)
           printf("%s was not the known name %s, ignored.\n", from, unique_name);
        else
           ecore_event_add(ELOCATION_EVENT_OUT, NULL, NULL, NULL);
     }
   else
     {
        printf("unknow change from %s to %s\n", from, to);
     }
}

static void
_get_name_owner(void *data , DBusMessage *msg, DBusError *err)
{
   DBusMessageIter itr;
   const char *uid;

   pending_get_name_owner = NULL;

   if (!dbus_message_iter_init(msg, &itr))
      return;

   dbus_message_iter_get_basic(&itr, &uid);
   if (!uid)
        return;

   printf("enter GeoClue at %s (old was %s)\n", uid, unique_name);
   if (unique_name && strcmp(unique_name, uid) == 0)
     {
        return;
     }

   if (unique_name)
      ecore_event_add(ELOCATION_EVENT_IN, NULL, NULL, NULL);

   unique_name = strdup(uid);
   return;
}

static void
status_cb(void *data , DBusMessage *reply, DBusError *error)
{
   dbus_int32_t status;
   DBusMessageIter iter;

   if (!dbus_message_has_signature(reply, "i")) return;

   dbus_message_iter_init(reply, &iter);
   dbus_message_iter_get_basic(&iter, &status);

   master_provider.status = status;

   printf("Status: %i\n", master_provider.status);
}

static void
status_signal_cb(void *data , DBusMessage *reply)
{
   dbus_int32_t status;
   DBusMessageIter iter;

   if (!dbus_message_has_signature(reply, "i")) return;

   dbus_message_iter_init(reply, &iter);
   dbus_message_iter_get_basic(&iter, &status);

   ecore_event_add(ELOCATION_EVENT_STATUS, &status, NULL, NULL);
   master_provider.status = status;
}

Eina_Bool
elocation_init(E_DBus_Connection *conn)
{
   DBusMessage *msg;

   if (ELOCATION_EVENT_IN == 0)
      ELOCATION_EVENT_IN = ecore_event_type_new();

   if (ELOCATION_EVENT_OUT == 0)
      ELOCATION_EVENT_OUT = ecore_event_type_new();

   if (ELOCATION_EVENT_STATUS == 0)
      ELOCATION_EVENT_STATUS = ecore_event_type_new();

   if (ELOCATION_EVENT_STATUS == 0)
      ELOCATION_EVENT_STATUS = ecore_event_type_new();

   if (ELOCATION_EVENT_ADDRESS == 0)
      ELOCATION_EVENT_ADDRESS = ecore_event_type_new();

   cb_name_owner_changed = e_dbus_signal_handler_add
         (conn, E_DBUS_FDO_BUS, E_DBUS_FDO_PATH, E_DBUS_FDO_INTERFACE, "NameOwnerChanged", _system_name_owner_changed, NULL);

   if (pending_get_name_owner)
      dbus_pending_call_cancel(pending_get_name_owner);

   pending_get_name_owner = e_dbus_get_name_owner(conn, GEOCLUE_DBUS_NAME, _get_name_owner, NULL);

   ecore_event_handler_add(ELOCATION_EVENT_IN, geoclue_start, NULL);
   ecore_event_handler_add(ELOCATION_EVENT_OUT, geoclue_stop, NULL);

   msg = dbus_message_new_method_call(GEOCLUE_DBUS_NAME, GEOCLUE_OBJECT_PATH, GEOCLUE_DBUS_NAME, "Create");
   e_dbus_message_send(conn, msg, create_cb, -1, NULL);
   dbus_message_unref(msg);
   msg = NULL;

   msg = dbus_message_new_method_call(UBUNTU_DBUS_NAME, UBUNTU_OBJECT_PATH, GEOCLUE_IFACE, "GetStatus");
   e_dbus_message_send(conn, msg, status_cb, -1, NULL);
   dbus_message_unref(msg);
   msg = NULL;

   e_dbus_signal_handler_add(conn, UBUNTU_DBUS_NAME, UBUNTU_OBJECT_PATH, GEOCLUE_POSITION_IFACE, "GetStatus",
         status_signal_cb, NULL);
}

void
elocation_shutdown(E_DBus_Connection *conn)
{
   if (pending_get_name_owner)
     {
        dbus_pending_call_cancel(pending_get_name_owner);
        pending_get_name_owner = NULL;
     }

   if (cb_name_owner_changed)
     {
        e_dbus_signal_handler_del(conn, cb_name_owner_changed);
        cb_name_owner_changed = NULL;
     }
}
