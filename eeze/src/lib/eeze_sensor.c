#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <Eina.h>
#include <Ecore.h>
#include <Eeze_Sensor.h>
#include "eeze_sensor_private.h"

Eeze_Sensor *g_handle;

/* Priority order for modules. The one with the highest order of the available
 * ones will be used. This in good enough for now as we only have two modules
 * and one is a test harness anyway. If the number of modules grows we might
 * re-think the priority handling, but we should do this when the need arise.
 */
static const char *_module_priority[] = {
   "tizen",
   "fake",
   NULL
};

/* Search through the list of loaded module and return the one with the highest
 * priority.
 */
Eeze_Sensor_Module *
_highest_priority_module_get(void)
{
   Eeze_Sensor_Module *module = NULL;
   int i = 0;

   while (_module_priority[i] != NULL)
     {
        module = eina_hash_find(g_handle->modules, _module_priority[i]);
        if (module) return module;
        i++;
     }
   return NULL;
}

/* Utility function to take the given sensor type and get the matching sensor
 * object from the highest priority module.
 */
EAPI Eeze_Sensor_Obj *
eeze_sensor_obj_get(Eeze_Sensor_Type sensor_type)
{
   Eina_List *l;
   Eeze_Sensor_Obj *obj, *sens;
   Eeze_Sensor_Module *module;

   module = _highest_priority_module_get();

   if (!module) return NULL;

   EINA_LIST_FOREACH(module->sensor_list, l, obj)
     {
        if (obj->type == sensor_type)
          {
             sens = calloc(1, sizeof(Eeze_Sensor_Obj));
             if (!sens) return NULL;

             memcpy(sens, obj, sizeof(Eeze_Sensor_Obj));

             return sens;
          }
     }
   return NULL;
}

static void
eeze_sensor_modules_load(void)
{
   /* Check for available runtime modules and load them. In some cases the
    * un-installed modules to be used from the local build dir. Coverage check
    * is one of these items. We do load the modules from the builddir if the
    * environment is set. Normal case is to use installed modules from system
    */
   if (getenv("EEZE_USE_IN_TREE_MODULES"))
      g_handle->modules_array = eina_module_list_get(NULL, PACKAGE_BUILD_DIR "/src/modules/.libs/", 0, NULL, NULL);
   else
      g_handle->modules_array = eina_module_list_get(NULL, PACKAGE_LIB_DIR "/eeze-sensor/", 0, NULL, NULL);

   if (!g_handle->modules_array)
     {
        ERR("No modules found!");
        return;
     }
   eina_module_list_load(g_handle->modules_array);
}

static void
eeze_sensor_modules_unload(void)
{
   if (!g_handle->modules_array) return;
   eina_module_list_unload(g_handle->modules_array);
   eina_module_list_free(g_handle->modules_array);
   eina_array_free(g_handle->modules_array);
   g_handle->modules_array = NULL;
}

/* This function is offered to the modules to register itself after they have
 * been loaded in initialized. They stay in the hash funtion until they
 * unregister themself.
 */
Eina_Bool
eeze_sensor_module_register(const char *name, Eeze_Sensor_Module *mod)
{
   Eeze_Sensor_Module *module = NULL;

   if (!mod) return EINA_FALSE;

   module = calloc(1, sizeof(Eeze_Sensor_Module));
   if (!module) return EINA_FALSE;

   module = mod;

   if (!module->init) return EINA_FALSE;
   if (!(module->init())) return EINA_FALSE;

   INF("Registered module %s", name);

   return eina_hash_add(g_handle->modules, name, module);
}

/* This function is offered to the modules to unregsiter itself. When requested
 * we remove them safely from the hash.
 */
Eina_Bool
eeze_sensor_module_unregister(const char *name)
{
   DBG("Unregister module %s", name);

   Eeze_Sensor_Module *module = NULL;

   module = eina_hash_find(g_handle->modules, name);
   if (module->shutdown)
     module->shutdown();

   return eina_hash_del(g_handle->modules, name, NULL);
}

/* Create a new sensor object for a given sensor type. This functions allocates
 * the needed memory and links it with the matching sensor from the loaded
 * modules. It also does an initial synchronous read to fill the sensor object
 * with values.
 * Make sure to use the eeze_sensor_free function to remove this sensor object
 * when it is no longer needed.
 */
EAPI Eeze_Sensor_Obj *
eeze_sensor_new(Eeze_Sensor_Type type)
{
   Eeze_Sensor_Obj *sens;
   Eeze_Sensor_Module *module = NULL;

   sens = eeze_sensor_obj_get(type);
   if (!sens) return NULL;

   module = _highest_priority_module_get();
   if (!module)
     {
      free(sens);
      return EINA_FALSE;
     }

   if (!module->read)
     {
      free(sens);
      return NULL;
     }

   /* The read is asynchronous here as we want to make sure that the sensor
    * object has valid data when created. As we give back cached values we
    * have a race condition when we do a asynchronous read here and the
    * application asks for cached data before the reply came in. This logic has
    * the downside that the sensor creation takes longer. But that is only a
    *initial cost.
    */
   if (module->read(sens->type, sens))
      return sens;

   free(sens);
   return NULL;
}

/* Free sensor object created with eeze_sensor_new */
EAPI void
eeze_sensor_free(Eeze_Sensor_Obj *sens)
{
   if (!sens) return;
   free(sens);
}

/* All of the below getter function do access the cached data from the last
 * sensor read. It is way faster this way but also means that the timestamp
 * should be checked to ensure recent data if needed.
 */
EAPI Eina_Bool
eeze_sensor_accuracy_get(Eeze_Sensor_Obj *sens, int *accuracy)
{
   if (!sens) return EINA_FALSE;

   *accuracy = sens->accuracy;
   return EINA_TRUE;
}

EAPI Eina_Bool
eeze_sensor_xyz_get(Eeze_Sensor_Obj *sens, float *x, float *y, float *z)
{
   if (!sens) return EINA_FALSE;

   *x = sens->data[0];
   *y = sens->data[1];
   *z = sens->data[2];
   return EINA_TRUE;
}

EAPI Eina_Bool
eeze_sensor_xy_get(Eeze_Sensor_Obj *sens, float *x, float *y)
{
   if (!sens) return EINA_FALSE;

   *x = sens->data[0];
   *y = sens->data[1];
   return EINA_TRUE;
}

EAPI Eina_Bool
eeze_sensor_x_get(Eeze_Sensor_Obj *sens, float *x)
{
   if (!sens) return EINA_FALSE;

   *x = sens->data[0];
   return EINA_TRUE;
}

EAPI Eina_Bool
eeze_sensor_timestamp_get(Eeze_Sensor_Obj *sens, unsigned long long *timestamp)
{
   if (!sens) return EINA_FALSE;

   *timestamp = sens->timestamp;
   return EINA_TRUE;
}

/* Synchronous read. Blocked until the data was readout from the hardware
 * sensor
 */
EAPI Eina_Bool
eeze_sensor_read(Eeze_Sensor_Obj *sens)
{
   Eeze_Sensor_Module *module = NULL;

   if (!sens) return EINA_FALSE;

   module = _highest_priority_module_get();
   if (!module) return EINA_FALSE;

   if (module->read)
     return module->read(sens->type, sens);

   return EINA_FALSE;
}

/* Asynchronous read. Schedule a new read out that will update the cached values
 * as soon as it arrives.
 */
EAPI Eina_Bool
eeze_sensor_async_read(Eeze_Sensor_Obj *sens, void *user_data)
{
   Eeze_Sensor_Module *module = NULL;

   if (!sens) return EINA_FALSE;

   module = _highest_priority_module_get();
   if (!module) return EINA_FALSE;
   if (module->async_read)
     return module->async_read(sens->type, user_data);

   return EINA_FALSE;
}

EAPI void
eeze_sensor_shutdown(void)
{
   eeze_sensor_modules_unload();
   eina_hash_free(g_handle->modules);
   g_handle->modules = NULL;
   free(g_handle);
   g_handle = NULL;

   eina_shutdown();
}

EAPI Eina_Bool
eeze_sensor_init(void)
{
   if (!eina_init()) return EINA_FALSE;

   g_handle = calloc(1, sizeof(Eeze_Sensor));
   if (!g_handle) return EINA_FALSE;

   g_handle->modules_array = NULL;
   g_handle->modules = eina_hash_string_small_new(NULL);
   if (!g_handle->modules) return EINA_FALSE;

   /* Make sure we create new ecore event types before using them */
   EEZE_SENSOR_EVENT_SNAP = ecore_event_type_new();
   EEZE_SENSOR_EVENT_SHAKE = ecore_event_type_new();
   EEZE_SENSOR_EVENT_DOUBLETAP = ecore_event_type_new();
   EEZE_SENSOR_EVENT_PANNING = ecore_event_type_new();
   EEZE_SENSOR_EVENT_FACEDOWN = ecore_event_type_new();
   EEZE_SENSOR_EVENT_ACCELEROMETER = ecore_event_type_new();

   /* Core is ready so we can load the modules from disk now */
   eeze_sensor_modules_load();

   return EINA_TRUE;
}
