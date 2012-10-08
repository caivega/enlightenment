#ifndef _ELOCATION_H
#define _ELOCATION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EFL_ECORE_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_ECORE_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

#include <stdio.h>

#include <Ecore.h>
#include <EDBus.h>

/**
 * @defgroup Location_Events Available location events
 * @brief Location events that are emitted from the library
 * @since 1.8
 * @{
 */
EAPI extern int ELOCATION_EVENT_STATUS;
EAPI extern int ELOCATION_EVENT_POSITION;
EAPI extern int ELOCATION_EVENT_ADDRESS;
/**@}*/

/**
 * @typedef Elocation_Accuracy_Level
 * @since 1.8
 *
 * Different location accuraccy levels from country up to detailed,
 * e.g. GPS, information.
 */
typedef enum {
   ELOCATION_ACCURACY_LEVEL_NONE = 0,
   ELOCATION_ACCURACY_LEVEL_COUNTRY,
   ELOCATION_ACCURACY_LEVEL_REGION,
   ELOCATION_ACCURACY_LEVEL_LOCALITY,
   ELOCATION_ACCURACY_LEVEL_POSTALCODE,
   ELOCATION_ACCURACY_LEVEL_STREET,
   ELOCATION_ACCURACY_LEVEL_DETAILED,
} Elocation_Accuracy_Level;

/**
 * @typedef Elocation_Resource_Flags
 * @since 1.8
 *
 * Flags for available system resources to be used for locating.
 */
typedef enum {
   ELOCATION_RESOURCE_NONE = 0,
   ELOCATION_RESOURCE_NETWORK = 1 << 0,
   ELOCATION_RESOURCE_CELL = 1 << 1,
   ELOCATION_RESOURCE_GPS = 1 << 2,

   ELOCATION_RESOURCE_ALL = (1 << 10) - 1
} Elocation_Resource_Flags;

/**
 * @typedef Elocation_Accuracy
 * @since 1.8
 *
 * Information about the accurancy of the reported location.
 */
typedef struct _Elocation_Accuracy
{
   int level;
   double horizontal;
   double vertical;
} Elocation_Accuracy;

/**
 * @typedef Elocation_Address
 * @since 1.8
 *
 * Location information in textual form. Depending on the used provider this
 * can cover only the country or a detailed address based on GPS information.
 */
typedef struct _Elocation_Address
{
   unsigned int timestamp;
   char *country;
   char *countrycode;
   char *locality;
   char *postalcode;
   char *region;
   char *timezone;
   Elocation_Accuracy *accur;
} Elocation_Address;

/**
 * @typedef Elocation_Position
 * @since 1.8
 *
 * Location information based on the GPS grid. Latitude, longitude and altitude.
 */
typedef struct _Elocation_Postion
{
   unsigned int timestamp;
   double latitude;
   double longitude;
   double altitude;
   Elocation_Accuracy *accur;
} Elocation_Position;

/**
 * @typedef Elocation_Requirements
 * @since 1.8
 *
 * Requirement settings for the location provider. Requirements can be an level
 * of accurancy or allowed resources like network access or GPS.
 */
typedef struct _Elocation_Requirements
{
   Elocation_Accuracy_Level accurancy_level;
   int time;
   Eina_Bool require_update;
   Elocation_Resource_Flags allowed_resources;
} Elocation_Requirements;

/**
 * @brief Get the current address information.
 * @param address Address struct to be filled with information.
 * @return EINA_TRUE for success and EINA_FALSE for failure.
 *
 * @since 1.8
 */
EAPI Eina_Bool elocation_address_get(Elocation_Address *address);

/**
 * @brief Get the current position information.
 * @param position Position struct to be filled with information.
 * @return EINA_TRUE for success and EINA_FALSE for failure.
 *
 * @since 1.8
 */
EAPI Eina_Bool elocation_position_get(Elocation_Position *position);

/**
 * @brief Get the current status.
 * @param status Status
 * @return EINA_TRUE for success and EINA_FALSE for failure.
 *
 * @since 1.8
 */
EAPI Eina_Bool elocation_status_get(int *status);

/**
 * @brief Set the requirements.
 * @param requirements Requirements
 * @return EINA_TRUE for success and EINA_FALSE for failure.
 *
 * @since 1.8
 */
EAPI Eina_Bool elocation_requirements_set(Elocation_Requirements *requirements);

EAPI Eina_Bool elocation_init();
EAPI void elocation_shutdown();
#endif
