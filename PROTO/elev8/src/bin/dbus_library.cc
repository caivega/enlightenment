#include "dbus_library.h"
#include <Eina.h>
#include <E_DBus.h>
#include <Ecore.h>

using namespace v8;
int elev8_dbus_log_domain = -1;

Handle<ObjectTemplate> dbusObj;

static Eina_Bool cb_parse_method_argument_attributes(void *data, const char *key, const char *value)
{
   struct DBus_Method_Argument *ma = (struct DBus_Method_Argument *)data;

   if (strcmp(key, "name") == 0)
     eina_stringshare_replace(&ma->name, value);
   else if (strcmp(key, "type") == 0)
     eina_stringshare_replace(&ma->type, value);
   else if (strcmp(key, "direction") == 0)
     {
        if (strcmp(value, "out") == 0)
          ma->is_output = EINA_TRUE;
        else if (strcmp(value, "in") == 0)
          ma->is_output = EINA_FALSE;
        else
          {
             EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: unknown method argument direction %s",
                     value);
             return EINA_FALSE;
          }
     }
   else
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: unknown method argument attribute %s=%s",
                key, value);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool parse_method_argument(struct DBus_Method *m, const char *attrs, unsigned length)
{
   struct DBus_Method_Argument *ma;

   ma = (struct DBus_Method_Argument *)calloc(1, sizeof(struct DBus_Method_Argument));
   if (!ma)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not allocate memory");
        return EINA_FALSE;
     }

   if (!eina_simple_xml_attributes_parse
       (attrs, length, cb_parse_method_argument_attributes, ma))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not parse method argument attributes");
        eina_stringshare_del(ma->name);
        eina_stringshare_del(ma->type);
        free(ma);
        return EINA_FALSE;
     }

   if (!ma->type)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: method argument must have a type!");
        eina_stringshare_del(ma->name);
        eina_stringshare_del(ma->type);
        free(ma);
        return EINA_FALSE;
     }

   m->arguments = eina_inlist_append(m->arguments, EINA_INLIST_GET(ma));
   return EINA_TRUE;
}

static void free_method_argument(struct DBus_Method_Argument *ma)
{
   eina_stringshare_del(ma->type);
   eina_stringshare_del(ma->name);
   free(ma);
}

static Eina_Bool cb_parse_method_attributes(void *data, const char *key, const char *value)
{
   struct DBus_Method *m = (struct DBus_Method *)data;
   if (strcmp(key, "name") != 0) return EINA_FALSE;

   eina_stringshare_replace(&m->name, value);
   return EINA_TRUE;
}

static struct DBus_Method *parse_method(const char *attrs, unsigned length)
{
   struct DBus_Method *m = (struct DBus_Method *)calloc(1, sizeof(struct DBus_Method));
   if (!m)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not allocate memory");
        return NULL;
     }

   if (!eina_simple_xml_attributes_parse
       (attrs, length, cb_parse_method_attributes, m))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not parse method attributes");
        free(m);
        return NULL;
     }

   if (!m->name)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: method must have a name!");
        free(m);
        return NULL;
     }

   return m;
}

static void free_method(struct DBus_Method *m)
{
   while (m->arguments)
     {
        struct DBus_Method_Argument *tmp = EINA_INLIST_CONTAINER_GET
          (m->arguments, struct DBus_Method_Argument);
        m->arguments = m->arguments->next;
        free_method_argument(tmp);
     }

   eina_stringshare_del(m->name);
   free(m);
}

static Eina_Bool cb_parse_signal_argument_attributes(void *data, const char *key, const char *value)
{
   struct DBus_Signal_Argument *sa = (struct DBus_Signal_Argument *)data;

   if (strcmp(key, "name") == 0)
     eina_stringshare_replace(&sa->name, value);
   else if (strcmp(key, "type") == 0)
     eina_stringshare_replace(&sa->type, value);
   else
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: unknown signal argument attribute %s=%s",
                key, value);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool parse_signal_argument(struct DBus_Signal *s, const char *attrs, unsigned length)
{
   struct DBus_Signal_Argument *sa;

   sa = (struct DBus_Signal_Argument *)calloc(1, sizeof(struct DBus_Signal_Argument));
   if (!sa)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not allocate memory");
        return EINA_FALSE;
     }

   if (!eina_simple_xml_attributes_parse
       (attrs, length, cb_parse_signal_argument_attributes, sa))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not parse signal argument attributes");
        eina_stringshare_del(sa->name);
        eina_stringshare_del(sa->type);
        free(sa);
        return EINA_FALSE;
     }

   if (!sa->type)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: signal argument must have a type!");
        eina_stringshare_del(sa->name);
        eina_stringshare_del(sa->type);
        free(sa);
        return EINA_FALSE;
     }

   s->arguments = eina_inlist_append(s->arguments, EINA_INLIST_GET(sa));
   return EINA_TRUE;
}

static void free_signal_argument(struct DBus_Signal_Argument *sa)
{
   eina_stringshare_del(sa->type);
   eina_stringshare_del(sa->name);
   free(sa);
}

static Eina_Bool cb_parse_signal_attributes(void *data, const char *key, const char *value)
{
   struct DBus_Signal *s = (struct DBus_Signal *)data;
   if (strcmp(key, "name") != 0) return EINA_FALSE;

   eina_stringshare_replace(&s->name, value);
   return EINA_TRUE;
}

static struct DBus_Signal *parse_signal(const char *attrs, unsigned length)
{
   struct DBus_Signal *s = (struct DBus_Signal *)calloc(1, sizeof(struct DBus_Signal));
   if (!s)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not allocate memory");
        return NULL;
     }

   if (!eina_simple_xml_attributes_parse
       (attrs, length, cb_parse_signal_attributes, s))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not parse signal attributes");
        free(s);
        return NULL;
     }

   if (!s->name)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: signal must have a name!");
        free(s);
        return NULL;
     }

   return s;
}

static void free_signal(struct DBus_Signal *s)
{
   while (s->arguments)
     {
        struct DBus_Signal_Argument *tmp = EINA_INLIST_CONTAINER_GET
          (s->arguments, struct DBus_Signal_Argument);
        s->arguments = s->arguments->next;
        free_signal_argument(tmp);
     }

   eina_stringshare_del(s->name);
   free(s);
}

static Eina_Bool cb_parse_property_attributes(void *data, const char *key, const char *value)
{
   struct DBus_Property *p = (struct DBus_Property *)data;
   if (strcmp(key, "name") == 0)
     eina_stringshare_replace(&p->name, value);
   else if (strcmp(key, "type") == 0)
     eina_stringshare_replace(&p->type, value);
   else if (strcmp(key, "access") == 0)
     {
        if (strcmp(value, "read") == 0)
          {
             p->can_read = EINA_TRUE;
             p->can_write = EINA_FALSE;
          }
        else if (strcmp(value, "write") == 0)
          {
             p->can_read = EINA_FALSE;
             p->can_write = EINA_TRUE;
          }
        else if (strcmp(value, "readwrite") == 0)
          {
             p->can_read = EINA_TRUE;
             p->can_write = EINA_TRUE;
          }
        else
          {
             EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: unknown access value '%s'", value);
             return EINA_FALSE;
          }
     }
   else
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: unknown property attribute %s=%s",
                key, value);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static struct DBus_Property *parse_property(const char *attrs, unsigned length)
{
   struct DBus_Property *p = (struct DBus_Property *)calloc(1, sizeof(struct DBus_Property));
   if (!p)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not allocate memory");
        return NULL;
     }

   p->can_read = EINA_TRUE;
   p->can_write = EINA_TRUE;
   if (!eina_simple_xml_attributes_parse
       (attrs, length, cb_parse_property_attributes, p))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not parse property attributes");
        free(p);
        return NULL;
     }

   if (!p->name)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: property must have a name!");
        free(p);
        return NULL;
     }

   return p;
}

static void free_property(struct DBus_Property *p)
{
   eina_stringshare_del(p->name);
   eina_stringshare_del(p->type);
   free(p);
}

static Eina_Bool cb_parse_interface_attributes(void *data, const char *key, const char *value)
{
   struct DBus_Interface *i = (struct DBus_Interface *)data;
   if (strcmp(key, "name") != 0) return EINA_FALSE;

   eina_stringshare_replace(&i->name, value);

   return EINA_TRUE;
}

static struct DBus_Interface *parse_interface(const char *attrs, unsigned length)
{
   struct DBus_Interface *i = (struct DBus_Interface *)calloc
                                        (1,sizeof(struct DBus_Interface));
   if (!i)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not allocate memory");
        return NULL;
     }

   if (!eina_simple_xml_attributes_parse
       (attrs, length, cb_parse_interface_attributes, i))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not parse interface attributes");
        free(i);
        return NULL;
     }

   if (!i->name)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: interface must have a name!");
        free(i);
        return NULL;
     }

   return i;
}

static void free_interface(struct DBus_Interface *i)
{
   while (i->methods)
     {
        struct DBus_Method *tmp = EINA_INLIST_CONTAINER_GET
          (i->methods, struct DBus_Method);
        i->methods = i->methods->next;
        free_method(tmp);
     }

   while (i->signals)
     {
        struct DBus_Signal *tmp = EINA_INLIST_CONTAINER_GET
          (i->signals, struct DBus_Signal);
        i->signals = i->signals->next;
        free_signal(tmp);
     }

   while (i->properties)
     {
        struct DBus_Property *tmp = EINA_INLIST_CONTAINER_GET
          (i->properties, struct DBus_Property);
        i->properties = i->properties->next;
        free_property(tmp);
     }

   eina_stringshare_del(i->name);
   free(i);
}

static Eina_Bool cb_parse_node_attributes(void *data, const char *key, const char *value)
{
   struct DBus_Node *n = (struct DBus_Node *)data;
   if (strcmp(key, "name") != 0) return EINA_FALSE;

   eina_stringshare_replace(&n->name, value);
   return EINA_TRUE;
}

static struct DBus_Node *parse_node(const char *attrs, unsigned length)
{
   struct DBus_Node *n = (struct DBus_Node *)calloc(1, sizeof(struct DBus_Node));
   if (!n)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not allocate memory");
        return NULL;
     }

   if (!attrs) return n;

   if (!eina_simple_xml_attributes_parse
       (attrs, length, cb_parse_node_attributes, n))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not parse node attributes");
        free(n);
        return NULL;
     }
   return n;
}

static void free_node(struct DBus_Node *n)
{
   while (n->children)
     {
        struct DBus_Node *tmp = EINA_INLIST_CONTAINER_GET
          (n->children, struct DBus_Node);
        n->children = n->children->next;
        free_node(tmp);
     }

   while (n->interfaces)
     {
        struct DBus_Interface *tmp = EINA_INLIST_CONTAINER_GET
          (n->interfaces, struct DBus_Interface);
        n->interfaces = n->interfaces->next;
        free_interface(tmp);
     }

   eina_stringshare_del(n->name);
   free(n);
}

static Eina_Bool cb_parse(void *data, Eina_Simple_XML_Type type, const char *content, unsigned offset, unsigned length)
{
   struct DBus_Introspection_Parse_Ctxt *ctxt = (struct DBus_Introspection_Parse_Ctxt *)data;

   switch (type)
     {
      case EINA_SIMPLE_XML_OPEN:
      case EINA_SIMPLE_XML_OPEN_EMPTY:
        {
           /* split tag name and attributes */
           const char *attrs = eina_simple_xml_tag_attributes_find
             (content, length);
           unsigned attrslen = 0;
           int sz = length;
           if (attrs)
             {
                attrslen = length - (attrs - content);
                sz = attrs - content;
                while ((sz > 0) && (isspace(content[sz - 1])))
                  sz--;
             }

           if (!ctxt->node)
             {
                if (strncmp("node", content, sz) != 0)
                  {
                     EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: expected <node>, got %.*s",
                             length, content);
                     return EINA_FALSE;
                  }

                ctxt->node = parse_node(attrs, attrslen);
                if (!ctxt->node) return EINA_FALSE;
                ctxt->nodes = eina_inlist_append
                  (ctxt->nodes, EINA_INLIST_GET(ctxt->node));
             }
           else
             {
                if (strncmp("node", content, sz) == 0)
                  {
                     struct DBus_Node *n = parse_node(attrs, attrslen);
                     if (!n) return EINA_FALSE;


                     ctxt->node->children = eina_inlist_append
                       (ctxt->node->children, EINA_INLIST_GET(n));

                     if (type != EINA_SIMPLE_XML_OPEN_EMPTY)
                       ctxt->node = n;

                     EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Node Name = %s", n->name);
                  }
                else if (strncmp("interface", content, sz) == 0)
                  {
                     if (!ctxt->node)
                       {
                          EINA_LOG_DOM_ERR(elev8_dbus_log_domain,
                                  "Error: cannot have <%.*s> outside <node>!",
                                  length, content);
                          return EINA_FALSE;
                       }

                     ctxt->interface = parse_interface(attrs, attrslen);
                     if (!ctxt->interface) return EINA_FALSE;
                     EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Interface %s %p",
                                          ctxt->interface->name, ctxt->interface);
                     ctxt->node->interfaces = eina_inlist_append
                       (ctxt->node->interfaces, EINA_INLIST_GET(ctxt->interface));
                  }
                else if (strncmp("method", content, sz) == 0)
                  {
                     struct DBus_Method *m;

                     if (!ctxt->interface)
                       {
                          EINA_LOG_DOM_ERR(elev8_dbus_log_domain,
                                  "Error: cannot have <%.*s> outside "
                                  "<interface>!",
                                  length, content);
                          return EINA_FALSE;
                       }

                     m = parse_method(attrs, attrslen);
                     if (!m) return EINA_FALSE;
                     ctxt->interface->methods = eina_inlist_append
                       (ctxt->interface->methods, EINA_INLIST_GET(m));

                     if (type != EINA_SIMPLE_XML_OPEN_EMPTY)
                       ctxt->method = m;

                     EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Method Name = %s", m->name);
                  }
                else if (strncmp("signal", content, sz) == 0)
                  {
                     struct DBus_Signal *s;

                     if (!ctxt->interface)
                       {
                          EINA_LOG_DOM_ERR(elev8_dbus_log_domain,
                                  "Error: cannot have <%.*s> outside "
                                  "<interface>!",
                                  length, content);
                          return EINA_FALSE;
                       }

                     s = parse_signal(attrs, attrslen);
                     if (!s) return EINA_FALSE;
                     ctxt->interface->signals = eina_inlist_append
                       (ctxt->interface->signals, EINA_INLIST_GET(s));

                     if (type != EINA_SIMPLE_XML_OPEN_EMPTY)
                       ctxt->signal = s;

                     EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Signal Name = %s", s->name);
                  }
                else if (strncmp("property", content, sz) == 0)
                  {
                     struct DBus_Property *p;

                     if (!ctxt->interface)
                       {
                          EINA_LOG_DOM_ERR(elev8_dbus_log_domain,
                                  "Error: cannot have <%.*s> outside "
                                  "<interface>!",
                                  length, content);
                          return EINA_FALSE;
                       }

                     p = parse_property(attrs, attrslen);
                     if (!p) return EINA_FALSE;
                     ctxt->interface->properties = eina_inlist_append
                       (ctxt->interface->properties, EINA_INLIST_GET(p));

                     EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Property Name = %s Type = %s", p->name, p->type);
                  }
                else if (strncmp("arg", content, sz) == 0)
                  {
                     if (ctxt->method)
                       {
                          if (!parse_method_argument
                              (ctxt->method, attrs, attrslen))
                            {
                               EINA_LOG_DOM_ERR(elev8_dbus_log_domain,
                                       "Error: cannot parse method's <%.*s>",
                                       length, content);
                               return EINA_FALSE;
                            }
                       }
                     else if (ctxt->signal)
                       {
                          if (!parse_signal_argument
                              (ctxt->signal, attrs, attrslen))
                            {
                               EINA_LOG_DOM_ERR(elev8_dbus_log_domain,
                                       "Error: cannot parse signal's <%.*s>",
                                       length, content);
                               return EINA_FALSE;
                            }
                       }
                     else
                       {
                          EINA_LOG_DOM_ERR(elev8_dbus_log_domain,
                                  "Error: cannot have <%.*s> outside "
                                  "<method> or <signal>!",
                                  length, content);
                          return EINA_FALSE;
                       }
                  }
                else
                  {
                     EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: unexpected element <%.*s>",
                             length, content);
                     return EINA_FALSE;
                  }
             }
        }
        break;
      case EINA_SIMPLE_XML_CLOSE:
         if (strncmp("node", content, length) == 0)
           ctxt->node = NULL;
         else if (strncmp("interface", content, length) == 0)
           ctxt->interface = NULL;
         else if (strncmp("method", content, length) == 0)
           ctxt->method = NULL;
         else if (strncmp("signal", content, length) == 0)
           ctxt->signal = NULL;
         break;
      case EINA_SIMPLE_XML_DATA:
      case EINA_SIMPLE_XML_CDATA:
      case EINA_SIMPLE_XML_PROCESSING:
      case EINA_SIMPLE_XML_DOCTYPE:
      case EINA_SIMPLE_XML_COMMENT:
      case EINA_SIMPLE_XML_IGNORED:
         /*EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Ignored: contents at offset %u-%u: %.*s",
                 offset, length, length, content);*/
         break;
      case EINA_SIMPLE_XML_ERROR:
         EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: parser error at offset %u-%u: %.*s",
                 offset, length, length, content);
         break;
     }
   return EINA_TRUE;
}

void invoke_js_callback(void *data)
{
   DBus *dbus = ((struct dbus_cache *)(data))->dbus;
   if (dbus->js_introspect_cb->IsFunction())
     {
        Local<Function> func = Function::Cast(*(dbus->js_introspect_cb));
        if (!func.IsEmpty())
          {
             Persistent<String> path = static_cast<Persistent<String> >(
                                        ((struct dbus_cache *)(data))->service
					                    );
             Handle<Value> args[1] = { path };
             func->Call(dbus->obj, 1, args);
          }
     }
}

static void cb_introspect(void *data, DBusMessage *msg, DBusError *error)
{
   DBusError e;
   DBus *dbus = ((struct dbus_cache *)(data))->dbus;
   const char *xml_str;

   if ((error) && (dbus_error_is_set(error)))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: DBus replied with error %s: %s",
                error->name, error->message);
        return;
     }

   dbus_error_init(&e);
   if (!dbus_message_get_args(msg, &e, DBUS_TYPE_STRING, &xml_str,
                              DBUS_TYPE_INVALID))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not get arguments %s: %s",
                e.name, e.message);
        return;
     }

   struct DBus_Introspection_Parse_Ctxt *ctxt;
   ctxt = (struct DBus_Introspection_Parse_Ctxt *)calloc(1, sizeof(struct DBus_Introspection_Parse_Ctxt));
   if (!eina_simple_xml_parse(xml_str, strlen(xml_str), EINA_TRUE, cb_parse, ctxt))
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Error: could not parse XML:\n%s", xml_str);
        if (ctxt) free(ctxt);
        return;
     }

   std::pair<std::map<const char *,DBus_Introspection_Parse_Ctxt *>::iterator,bool> ret;

   v8::String::Utf8Value service(((struct dbus_cache *)(data))->service);
   EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Service->Name = %s Context = %p", *service, ctxt);
   ret = dbus->cached_results.insert(
                             std::pair<const char *,DBus_Introspection_Parse_Ctxt *>
                             (*service, ctxt));
   // can't store the result, performance will be affected.
   if (ret.second==false)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Cannot cache dbus result.");
     }
   EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Introspect called %s %p",*service, ctxt);
   invoke_js_callback(data);
}

Handle<Value> dbus_msg_introspect(const Arguments &args)
{
   HandleScope scope;

   EINA_LOG_DOM_INFO(elev8_dbus_log_domain,"Calling Introspect API");
   Local<Object> self = args.Holder();
   Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
   void *ptr = wrap->Value();
   DBus *dbus = (DBus *)ptr;

   if (args[0]->IsString() && args[1]->IsString())
     {
        String::Utf8Value service(args[0]->ToString());
        String::Utf8Value path(args[1]->ToString());
        EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "%s-%s", *service, *path);

        /* check if the bus has already been introspected */

        EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Looking for %s", *service);
        std::map<const char *,DBus_Introspection_Parse_Ctxt *>::iterator it;
        it = dbus->cached_results.find(*service);

        if (it==dbus->cached_results.end())
          {
             DBusPendingCall *pc;
             struct dbus_cache *dc = (struct dbus_cache *)calloc(1, sizeof(struct dbus_cache));
             dc->dbus = dbus;
             dc->service = v8::String::New(*service);
             pc = e_dbus_introspect(dbus->conn, *service, *path, cb_introspect, dc);

             if (!pc)
               {
                  EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Cannot introspect the given path");
               }
             EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Introspect called %s",*service);
          }
        else
          {
             EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Introspect Already Done. Returning Cached Result");
             invoke_js_callback(ptr);
          }
     }
   return Undefined();
}

void on_introspect(Local<String> property,
                                Local<Value> value,
                                const AccessorInfo& info)
{
   Local<Object> self = info.Holder();
   Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
   void *ptr = wrap->Value();
   DBus *dbus = (DBus *)ptr;

   String::Utf8Value prop_name(property);
   dbus->js_introspect_cb.Dispose();
   dbus->js_introspect_cb = Persistent<Value>::New(value);
   EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Introspect result set.");
}

Handle<Value> createDBusInstance(const Arguments& args)
{
   HandleScope scope;

   if (args[0]->IsString())
     {
        String::Utf8Value method(args[0]->ToString());
        if (!(strstr(*method, "System") || strstr(*method, "Session") 
                    ||  strstr(*method, "Starter")))
          {
             EINA_LOG_DOM_ERR(elev8_dbus_log_domain,"Undefined Session type");
             return Undefined();
          }
     }
   else
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain,"Please specify System or Session as parameter");
        return Undefined();
     }

   EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Creating DBUS Instance");
   DBus *dbus = new DBus();
   String::Utf8Value method(args[0]->ToString());

   /* After this step, dbus is integrated to main loop */
   if (strstr(*method, "System"))
      dbus->conn =  e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (strstr(*method, "Session"))
      dbus->conn =  e_dbus_bus_get(DBUS_BUS_SESSION);
   if (strstr(*method, "Starter"))
      dbus->conn =  e_dbus_bus_get(DBUS_BUS_STARTER);

   if (dbus->conn==NULL)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Cannot create DBus");
        delete dbus;
        return Undefined();
     }

   Local<FunctionTemplate> introspect = FunctionTemplate::New();

   introspect->SetCallHandler(dbus_msg_introspect);
   dbusObj->Set(String::New("introspect"), introspect);

   dbusObj->SetAccessor(String::New("on_introspect"),NULL, on_introspect, Null());

   dbus->obj.Dispose();
   dbus->obj = Persistent<Object>::New(dbusObj->NewInstance());
   dbus->obj->SetInternalField(0, External::New(dbus));

   EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "DBus Interface initialized");
   return dbus->obj; 
}

int dbus_v8_setup(Handle<ObjectTemplate> global)
{
   elev8_dbus_log_domain = eina_log_domain_register("elev8-dbus", EINA_COLOR_ORANGE);
   if (!elev8_dbus_log_domain)
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "could not register elev8-dbus log domain.");
        elev8_dbus_log_domain = EINA_LOG_DOMAIN_GLOBAL;
     }
   EINA_LOG_DOM_INFO(elev8_dbus_log_domain,"elev8-dbus Logging initialized. %d", elev8_dbus_log_domain);

   if (!e_dbus_init())
     {
        EINA_LOG_DOM_ERR(elev8_dbus_log_domain, "Cannot Init to E_DBus");
        return -1;
     }

   EINA_LOG_DOM_INFO(elev8_dbus_log_domain, "Creating DBus Instance");

   /* Add support for DBus Service Introspection */
   dbusObj = ObjectTemplate::New();
   dbusObj->SetInternalFieldCount(1);
   global->Set(String::New("dbus"), FunctionTemplate::New(createDBusInstance));
   return 0;
}
