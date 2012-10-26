#ifndef EDBUS_FREEDESKTOP_H
#define EDBUS_FREEDESKTOP_H 1

/**
 * @defgroup EDBus_Basic Basic Methods
 *
 * @{
 */
#define EDBUS_NAME_REQUEST_FLAG_ALLOW_REPLACEMENT 0x1 /**< Allow another service to become the primary owner if requested */
#define EDBUS_NAME_REQUEST_FLAG_REPLACE_EXISTING  0x2 /**< Request to replace the current primary owner */
#define EDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE      0x4 /**< If we can not become the primary owner do not place us in the queue */

/* Replies to request for a name */
#define EDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER    1 /**< Service has become the primary owner of the requested name */
#define EDBUS_NAME_REQUEST_REPLY_IN_QUEUE         2 /**< Service could not become the primary owner and has been placed in the queue */
#define EDBUS_NAME_REQUEST_REPLY_EXISTS           3 /**< Service is already in the queue */
#define EDBUS_NAME_REQUEST_REPLY_ALREADY_OWNER    4 /**< Service is already the primary owner */

EAPI EDBus_Pending *edbus_name_request(EDBus_Connection *conn, const char *bus, unsigned int flags, EDBus_Message_Cb cb, const void *cb_data);

/* Replies to releasing a name */
#define EDBUS_NAME_RELEASE_REPLY_RELEASED     1    /**< Service was released from the given name */
#define EDBUS_NAME_RELEASE_REPLY_NON_EXISTENT 2    /**< The given name does not exist on the bus */
#define EDBUS_NAME_RELEASE_REPLY_NOT_OWNER    3    /**< Service is not an owner of the given name */

EAPI EDBus_Pending *edbus_name_release(EDBus_Connection *conn, const char *bus, EDBus_Message_Cb cb, const void *cb_data);
EAPI EDBus_Pending *edbus_name_owner_get(EDBus_Connection *conn, const char *bus, EDBus_Message_Cb cb, const void *cb_data);
EAPI EDBus_Pending *edbus_name_owner_has(EDBus_Connection *conn, const char *bus, EDBus_Message_Cb cb, const void *cb_data);
EAPI EDBus_Pending *edbus_names_list(EDBus_Connection *conn, EDBus_Message_Cb cb, const void *cb_data);
EAPI EDBus_Pending *edbus_names_activatable_list(EDBus_Connection *conn, EDBus_Message_Cb cb, const void *cb_data);

/* Replies to service starts */
#define EDBUS_NAME_START_REPLY_SUCCESS         1 /**< Service was auto started */
#define EDBUS_NAME_START_REPLY_ALREADY_RUNNING 2 /**< Service was already running */

EAPI EDBus_Pending        *edbus_name_start(EDBus_Connection *conn, const char *bus, unsigned int flags, EDBus_Message_Cb cb, const void *cb_data);

typedef void (*EDBus_Name_Owner_Changed_Cb)(void *data, const char *bus, const char *old_id, const char *new_id);

/**
 * Add a callback to be called when unique id of a bus name changed.
 *
 * This function implicitly calls edbus_name_owner_get() in order to be able to
 * monitor the name. If the only interest is to receive notifications when the
 * name in fact changes, pass EINA_FALSE to @param allow_initial_call so your
 * callback will not be called on first retrieval of name owner. If the
 * initial state is important, pass EINA_TRUE to this parameter.
 *
 * @param conn connection
 * @param bus name of bus
 * @param cb callback
 * @param cb_data context data
 * @param allow_initial_call allow call callback with actual id of the bus
 */
EAPI void                  edbus_name_owner_changed_callback_add(EDBus_Connection *conn, const char *bus, EDBus_Name_Owner_Changed_Cb cb, const void *cb_data, Eina_Bool allow_initial_call);
/**
 * Remove callback added with edbus_name_owner_changed_callback_add().
 *
 * @param conn connection
 * @param bus name of bus
 * @param cb callback
 * @param cb_data context data
 */
EAPI void                  edbus_name_owner_changed_callback_del(EDBus_Connection *conn, const char *bus, EDBus_Name_Owner_Changed_Cb cb, const void *cb_data);

/**
 * @defgroup EDBus_FDO_Peer org.freedesktop.DBus.Peer
 *
 * @{
 */
EAPI EDBus_Pending        *edbus_object_peer_ping(EDBus_Object *obj, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1);
EAPI EDBus_Pending        *edbus_object_peer_machine_id_get(EDBus_Object *obj, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2);
/**
 * @}
 */

/**
 * @defgroup EDBus_FDO_Introspectable org.freedesktop.DBus.Introspectable
 *
 * @{
 */
EAPI EDBus_Pending        *edbus_object_introspect(EDBus_Object *obj, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2);
/**
 * @}
 */

/**
 * @defgroup EDBus_FDO_Properties org.freedesktop.DBus.Properties
 * @{
 */

/**
 * Enable or disable local cache of properties.
 *
 * After enable you can call edbus_proxy_property_local_get() or
 * edbus_proxy_property_local_get_all() to get cached properties.
 *
 * @note After enable, it will asynchrony get the properties values.
 */
EAPI void edbus_proxy_properties_monitor(EDBus_Proxy *proxy, Eina_Bool enable);

EAPI EDBus_Pending        *edbus_proxy_property_get(EDBus_Proxy *proxy, const char *name, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2, 3);
EAPI EDBus_Pending        *edbus_proxy_property_set(EDBus_Proxy *proxy, const char *name, char type, const void *value, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2, 4);
EAPI EDBus_Pending        *edbus_proxy_property_get_all(EDBus_Proxy *proxy, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2);
EAPI EDBus_Signal_Handler *edbus_proxy_properties_changed_callback_add(EDBus_Proxy *proxy, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2);

/**
 * Return the cached value of property.
 * This only work if you have enable edbus_proxy_properties_monitor or
 * if you have call edbus_proxy_event_callback_add of type
 * EDBUS_PROXY_EVENT_PROPERTY_CHANGED and the property you want had changed.
 */
EAPI Eina_Value           *edbus_proxy_property_local_get(EDBus_Proxy *proxy, const char *name) EINA_ARG_NONNULL(1, 2);

/**
 * Return a Eina_Hash with all cached properties.
 * This only work if you have enable edbus_proxy_properties_monitor or
 * if you have call edbus_proxy_event_callback_add of type
 * EDBUS_PROXY_EVENT_PROPERTY_CHANGED.
 */
EAPI const Eina_Hash      *edbus_proxy_property_local_get_all(EDBus_Proxy *proxy) EINA_ARG_NONNULL(1);
/**
 * @}
 */

/**
 * @defgroup EDBus_FDO_ObjectManager org.freedesktop.DBus.ObjectManager
 *
 * Whenever edbus_object_managed_objects_monitor() is called on an
 * object it will start listening for children being added or
 * interfaces changing on the object itself. It will then emit
 * events with edbus_object_event_type being
 * #EDBUS_OBJECT_EVENT_IFACE_ADDED,
 * #EDBUS_OBJECT_EVENT_IFACE_REMOVED,
 * #EDBUS_OBJECT_EVENT_PROPERTY_CHANGED and
 * #EDBUS_OBJECT_EVENT_PROPERTY_REMOVED.
 *
 * One may manually query the managed objects with
 * edbus_object_managed_objects_get() and listen for changes with
 * edbus_object_interfaces_added_callback_add() and
 * edbus_object_interfaces_removed_callback_add().
 *
 * @{
 */
EAPI void                  edbus_object_properties_monitor(EDBus_Object *obj);

EAPI EDBus_Pending        *edbus_object_managed_objects_get(EDBus_Object *obj, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2);
EAPI EDBus_Signal_Handler *edbus_object_interfaces_added_callback_add(EDBus_Object *obj, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2);
EAPI EDBus_Signal_Handler *edbus_object_interfaces_removed_callback_add(EDBus_Object *obj, EDBus_Message_Cb cb, const void *data) EINA_ARG_NONNULL(1, 2);
/**
 * @}
 */

/**
 * @}
 */
#endif
