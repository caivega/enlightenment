#ifndef ELEV8_HTTP_H
#define ELEV8_HTTP_H

#include <Ecore_Con.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <elev8_common.h>

#define HTTP_DBG(...) EINA_LOG_DOM_DBG(elev8_http_log_domain, __VA_ARGS__)
#define HTTP_INF(...) EINA_LOG_DOM_INFO(elev8_http_log_domain, __VA_ARGS__)
#define HTTP_WRN(...) EINA_LOG_DOM_WARN(elev8_http_log_domain, __VA_ARGS__)
#define HTTP_ERR(...) EINA_LOG_DOM_ERR(elev8_http_log_domain, __VA_ARGS__)
#define HTTP_CRT(...) EINA_LOG_DOM_CRITICAL(elev8_http_log_domain, __VA_ARGS__)


v8::Handle<v8::Value>response_text_getter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
v8::Handle<v8::Value>status_getter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
Eina_Bool completion_callback(void *data, int type, void *event);
Eina_Bool data_callback(void *data, int type, void *event);
v8::Handle<v8::Value> ecore_con_open(const v8::Arguments& args);
v8::Handle<v8::Value> get_response_header(const v8::Arguments& args);
v8::Handle<v8::Value> set_request_header(const v8::Arguments& args);
v8::Handle<v8::Value> ecore_con_send(const v8::Arguments& args);

enum 
{
	HTTP_GET,
	HTTP_POST
};

class XMLHttpRequest
{
   private:
      static int fd_counter;
   public:
      std::vector<std::string> responseHeaders;
      Ecore_Con_Url *url_con;
      Ecore_Event_Handler *url_complete_handle;
      Ecore_Event_Handler *url_data_handle;
      int http_method; //GET, POST
      Eina_Binbuf *data;
      v8::Local<v8::String> result;
      v8::Persistent<v8::Object> obj;
      v8::Persistent<v8::String> responseText;
      v8::Persistent<v8::Integer> status;
      v8::Persistent<v8::Integer> readyState;
      v8::Persistent<v8::String> localpath;
      v8::Persistent<v8::Value> onreadystatechange;

      XMLHttpRequest()
        {
          result = v8::String::Empty();
	  data = eina_binbuf_new();
        }
      ~XMLHttpRequest()
        {
	   eina_binbuf_free(data);
        }
      static int addFdCount()
        {
           fd_counter++;
	   return fd_counter;
	}

};

#endif
