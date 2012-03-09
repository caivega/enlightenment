#ifndef ELEV8_ELM_H
#define ELEV8_ELM_H

#include <Eina.h>
#include <Ecore.h>
#include <Elementary.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <v8.h>

#define ELM_DBG(...) EINA_LOG_DOM_DBG(elev8_elm_log_domain, __VA_ARGS__)
#define ELM_INF(...) EINA_LOG_DOM_INFO(elev8_elm_log_domain, __VA_ARGS__)
#define ELM_WRN(...) EINA_LOG_DOM_WARN(elev8_elm_log_domain, __VA_ARGS__)
#define ELM_ERR(...) EINA_LOG_DOM_ERR(elev8_elm_log_domain, __VA_ARGS__)
#define ELM_CRT(...) EINA_LOG_DOM_CRITICAL(elev8_elm_log_domain, __VA_ARGS__)

#define ELM_MODULE_NAME "elm"

#define FACTORY(type_) \
   public: \
      static CEvasObject* make(CEvasObject *parent, Local<Object> description) { \
         return new type_(parent, description); \
      }

extern int elev8_elm_log_domain;

#endif
