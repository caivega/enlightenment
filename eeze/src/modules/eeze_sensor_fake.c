#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <sys/time.h>

#include <Eina.h>
#include <Ecore.h>
#include <Eeze_Sensor.h>
#include "eeze_sensor_private.h"

/* This small Eeze_Sensor module is meant to be used as a test harness for
 * developing. It does not gather any real data from hardware sensors. It uses
 * fixed values for the data, but provides the correct timestamp value.
 */

Eeze_Sensor_Module *esensor_module;

Eina_Bool
fake_init(void)
{
   /* For the fake module we prepare a list with all potential sensors. Even if
    * we only have a small subset at the moment.
    */
   Eeze_Sensor_Type type;

   for (type = 0; type <= EEZE_SENSOR_TYPE_LAST; type++)
     {
        Eeze_Sensor_Obj *obj = calloc(1, sizeof(Eeze_Sensor_Obj));
        obj->type = type;
        esensor_module->sensor_list = eina_list_append(esensor_module->sensor_list, obj);
     }

   return EINA_TRUE;
}

/* We don't have anything to clear when we get unregistered from the core here.
 * This is different in other modules.
 */
Eina_Bool
fake_shutdown(void)
{
   return EINA_TRUE;
}

Eina_Bool
fake_read(Eeze_Sensor_Type sensor_type, Eeze_Sensor_Obj *lobj)
{
   Eeze_Sensor_Obj *obj = NULL;
   struct timeval tv;

   obj = eeze_sensor_obj_get(sensor_type);
   if (obj == NULL)
     {
        ERR("No matching sensor object found in list");
        return EINA_FALSE;
     }

   switch (sensor_type)
     {
      case EEZE_SENSOR_TYPE_ACCELEROMETER:
      case EEZE_SENSOR_TYPE_MAGNETIC:
      case EEZE_SENSOR_TYPE_ORIENTATION:
      case EEZE_SENSOR_TYPE_GYROSCOPE:
        /* This is only a test harness so we supply hardcoded values here */
        obj->accuracy = -1;
        obj->data[0] = 7;
        obj->data[1] = 23;
        obj->data[2] = 42;
        gettimeofday(&tv, NULL);
        obj->timestamp = ((tv.tv_sec * 1000000) + tv.tv_usec);
        break;

      case EEZE_SENSOR_TYPE_LIGHT:
      case EEZE_SENSOR_TYPE_PROXIMITY:
      case EEZE_SENSOR_TYPE_BAROMETER:
      case EEZE_SENSOR_TYPE_TEMPERATURE:
        obj->accuracy = -1;
        obj->data[0] = 7;
        gettimeofday(&tv, NULL);
        obj->timestamp = ((tv.tv_sec * 1000000) + tv.tv_usec);
        break;

      default:
        ERR("Not possible to read from this sensor type.");
        return EINA_FALSE;
     }

   memcpy(lobj, obj, sizeof(Eeze_Sensor_Obj));

   return EINA_TRUE;
}

Eina_Bool
fake_async_read(Eeze_Sensor_Type sensor_type, void *user_data EINA_UNUSED)
{
   Eeze_Sensor_Obj *obj = NULL;
   struct timeval tv;

   obj = eeze_sensor_obj_get(sensor_type);
   if (obj == NULL)
     {
        ERR("No matching sensor object found in list.");
        return EINA_FALSE;
     }

   switch (sensor_type)
     {
      case EEZE_SENSOR_TYPE_ACCELEROMETER:
         ecore_event_add(EEZE_SENSOR_EVENT_ACCELEROMETER, obj, NULL, NULL);
      case EEZE_SENSOR_TYPE_MAGNETIC:
      case EEZE_SENSOR_TYPE_ORIENTATION:
      case EEZE_SENSOR_TYPE_GYROSCOPE:
        obj->accuracy = -1;
        obj->data[0] = 7;
        obj->data[1] = 23;
        obj->data[2] = 42;
        gettimeofday(&tv, NULL);
        obj->timestamp = ((tv.tv_sec * 1000000) + tv.tv_usec);
        break;

      case EEZE_SENSOR_TYPE_LIGHT:
      case EEZE_SENSOR_TYPE_PROXIMITY:
      case EEZE_SENSOR_TYPE_BAROMETER:
      case EEZE_SENSOR_TYPE_TEMPERATURE:
        obj->accuracy = -1;
        obj->data[0] = 7;
        gettimeofday(&tv, NULL);
        obj->timestamp = ((tv.tv_sec * 1000000) + tv.tv_usec);
        break;

      default:
        ERR("Not possible to set a callback for this sensor type.");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/* This function gets called when the module is loaded from the disk. Its the
 * entry point to anything in this module. After settign ourself up we register
 * into the core of eeze sensor to make our functionality available.
 */
Eina_Bool
sensor_fake_init(void)
{
   /* Check to avoid multi-init */
   if (esensor_module) return EINA_FALSE;

   /* Set module function pointer to allow calls into the module */
   esensor_module = calloc(1, sizeof(Eeze_Sensor_Module));
   if (!esensor_module) return EINA_FALSE;

   /* Setup our function pointers to allow the core accessing this modules
    * functions
    */
   esensor_module->init = fake_init;
   esensor_module->shutdown = fake_shutdown;
   esensor_module->read = fake_read;
   esensor_module->async_read = fake_async_read;

   if (!eeze_sensor_module_register("fake", esensor_module))
     {
        ERR("Failed to register fake module.");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/* Cleanup when the module gets unloaded. Unregister ourself from the core to
 * avoid calls into a not loaded module.
 */
void
sensor_fake_shutdown(void)
{
   Eeze_Sensor_Obj *sens;

   eeze_sensor_module_unregister("fake");
   EINA_LIST_FREE(esensor_module->sensor_list, sens)
      free(sens);

   free(esensor_module);
   esensor_module = NULL;
}

EINA_MODULE_INIT(sensor_fake_init);
EINA_MODULE_SHUTDOWN(sensor_fake_shutdown);
