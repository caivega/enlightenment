#include <stdio.h>
#include <Ecore.h>
#include <Ecore_Ipc.h>
#include <Eina.h>
#include <Edje.h>
#include <Evas.h>
#include <Elementary.h>

#include "libclouseau.h"
#include "helper.h"           /*  Our own helper functions  */
#include "eet_dump.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
#define __UNUSED__
#endif


/* Globals */
static Elm_Genlist_Item_Class itc;
static Eina_List *tree = NULL;
static Eina_Bool list_show_clippers = EINA_TRUE, list_show_hidden = EINA_TRUE;

Eina_Bool
_add(void *data __UNUSED__, int type __UNUSED__, Ecore_Ipc_Event_Server_Add *ev)
{
   void *p;
   char *msg="Hello send from GUI client";
   int size = 0;

   ecore_ipc_server_data_size_max_set(ev->server, -1);

   ack_st t = { msg };
   p = packet_compose(GUI_ACK, &t, sizeof(t), &size);
   if (p)
     {
        ecore_ipc_server_send(ev->server, 0,0,0,0,EINA_FALSE, p, size);
        ecore_ipc_server_flush(ev->server);
        free(p);
     }

   return ECORE_CALLBACK_RENEW;
}


Eina_Bool
_del(void *data __UNUSED__, int type __UNUSED__, Ecore_Ipc_Event_Server_Del *ev)
{
   if (!ev->server)
     {
        printf("Failed to establish connection to the server.\nExiting.\n");
        ecore_main_loop_quit();
        return ECORE_CALLBACK_RENEW;
     }

   printf("Lost server with ip %s!\n", ecore_ipc_server_ip_get(ev->server));

   ecore_ipc_server_del(ev->server);

   ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}


static Eina_Bool
_load_gui_with_list(Evas_Object *gl, Eina_List *trees)
{
   Eina_List *l;
   Tree_Item *treeit;
   if (!trees)
     return EINA_TRUE;

   EINA_LIST_FOREACH(trees, l, treeit)
     {  /* Insert the base ee items */
        Elm_Genlist_Item_Type glflag = (treeit->children) ?
           ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE;
        elm_genlist_item_append(gl, &itc, treeit, NULL,
              glflag, NULL, NULL);
     }

   return EINA_TRUE;
}

Eina_Bool
_data(void *data, int type __UNUSED__, Ecore_Ipc_Event_Server_Data *ev)
{
   static Eina_Bool got_tree = EINA_FALSE;

   printf("Received %i bytes from server:\n"
         ">>>>>\n"
         "%%.%is\n"
         ">>>>>\n",
         ev->size, ev->size);

   Variant_st *v = packet_info_get(ev->data, ev->size);
   if (v)
     {
        switch(packet_mapping_type_get(v->t.type))
          {
           case DAEMON_TREE_DATA:
                {
                   printf("Got tree data from daemon, size=<%d>\n", ev->size);
                   elm_genlist_clear(data);
                   st_tree_list *tl = v->data;
                   _load_gui_with_list(data, tl->list);  /* data == gl */
                   break;
                }
          }

       /* variant_free(v); Will be saved as APP data, don't forget cleanup */
     }

   return ECORE_CALLBACK_RENEW;
}

static void
gl_exp(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   Evas_Object *gl = elm_object_item_widget_get(glit);
   Tree_Item *parent = elm_object_item_data_get(glit);
   Tree_Item *treeit;
   Eina_List *itr;

   EINA_LIST_FOREACH(parent->children, itr, treeit)
     {
        if ((!list_show_hidden && !treeit->is_visible) ||
              (!list_show_clippers && treeit->is_clipper))
           continue;

        Elm_Genlist_Item_Type iflag = (treeit->children) ?
           ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE;
        elm_genlist_item_append(gl, &itc, treeit, glit, iflag,
              NULL, NULL);
     }
}

static void
gl_con(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_subitems_clear(glit);
}

static void
gl_exp_req(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);
}

static void
gl_con_req(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);
}



static Evas_Object *
item_icon_get(void *data, Evas_Object *parent, const char *part)
{
   Tree_Item *treeit = data;

   if (!treeit->is_obj)
      return NULL;

   if (!strcmp(part, "elm.swallow.icon"))
     {
        char buf[PATH_MAX];

        if (treeit->is_clipper && !treeit->is_visible)
          {
             Evas_Object *ic;
             Evas_Object *bx = elm_box_add(parent);
             evas_object_size_hint_aspect_set(bx, EVAS_ASPECT_CONTROL_VERTICAL,
                   1, 1);

             ic = elm_icon_add(bx);
             snprintf(buf, sizeof(buf), "%s/images/clipper.png",
                   elm_app_data_dir_get());
             elm_icon_file_set(ic, buf, NULL);
             evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL,
                   1, 1);
             evas_object_size_hint_weight_set(ic, EVAS_HINT_EXPAND,
                   EVAS_HINT_EXPAND);
             evas_object_size_hint_align_set(ic, EVAS_HINT_FILL,
                   EVAS_HINT_FILL);
             elm_box_pack_end(bx, ic);

             ic = elm_icon_add(bx);
             snprintf(buf, sizeof(buf), "%s/images/hidden.png",
                   elm_app_data_dir_get());
             elm_icon_file_set(ic, buf, NULL);
             evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL,
                   1, 1);
             evas_object_size_hint_weight_set(ic, EVAS_HINT_EXPAND,
                   EVAS_HINT_EXPAND);
             evas_object_size_hint_align_set(ic, EVAS_HINT_FILL,
                   EVAS_HINT_FILL);
             elm_box_pack_end(bx, ic);

             return bx;

          }
        else if (treeit->is_clipper)
          {
             Evas_Object *ic;
             ic = elm_icon_add(parent);
             snprintf(buf, sizeof(buf), "%s/images/clipper.png",
                   elm_app_data_dir_get());
             elm_icon_file_set(ic, buf, NULL);
             evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL,
                   1, 1);
             return ic;
          }
        else if (!treeit->is_visible)
          {
             Evas_Object *ic;
             ic = elm_icon_add(parent);
             snprintf(buf, sizeof(buf), "%s/images/hidden.png",
                   elm_app_data_dir_get());
             elm_icon_file_set(ic, buf, NULL);
             evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL,
                   1, 1);
             return ic;
          }
     }

   return NULL;
}

static char *
item_text_get(void *data, Evas_Object *obj __UNUSED__,
      const char *part __UNUSED__)
{
   Tree_Item *treeit = data;
   char buf[256];
   snprintf(buf, sizeof(buf), "%p %s", treeit->ptr, treeit->name);
   return strdup(buf);
}

/* HIGHLIGHT code. */
/* The color of the highlight */
enum {
	HIGHLIGHT_R = 255,
	HIGHLIGHT_G = 128,
	HIGHLIGHT_B = 128,
	HIGHLIGHT_A = 255,

	/* How much padding around the highlight box.
         * Currently we don't want any. */
	PADDING = 0,
};

static Eina_Bool
libclouseau_highlight_fade(void *_rect)
{
   Evas_Object *rect = _rect;
   int r, g, b, a;
   double na;

   evas_object_color_get(rect, &r, &g, &b, &a);
   if (a < 20)
     {
        evas_object_del(rect);
        return EINA_FALSE;
     }

   na = a - 20;
   r = na / a * r;
   g = na / a * g;
   b = na / a * b;
   evas_object_color_set(rect, r, g, b, na);

   return EINA_TRUE;
}

static void
libclouseau_highlight(Evas_Object *obj)
{
   Evas *e;
   Evas_Object *r;
   int x, y, w, h;
   const char *tmp;

   e = evas_object_evas_get(obj);
   if (!e) return;

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   r = evas_object_rectangle_add(e);
   evas_object_move(r, x - PADDING, y - PADDING);
   evas_object_resize(r, w + (2 * PADDING), h + (2 * PADDING));
   evas_object_color_set(r, HIGHLIGHT_R, HIGHLIGHT_G, HIGHLIGHT_B,
         HIGHLIGHT_A);
   evas_object_show(r);
   ecore_timer_add(0.1, libclouseau_highlight_fade, r);

   tmp = evas_object_data_get(obj, ".clouseau.bt");
   fprintf(stderr, "Creation backtrace :\n%s*******\n", tmp);
}

static void
client_win_del(void *data, Evas_Object *obj, void *event_info)
{  /* called when client window is deleted, do cleanup here */
   /* TODO: client cleanup */
   elm_exit(); /* exit the program's main loop that runs in elm_run() */
}

static void
_gl_selected(void *data __UNUSED__, Evas_Object *pobj __UNUSED__,
      void *event_info)
{
//   clouseau_obj_information_list_clear();
   /* If not an object, return. */
   if (!elm_genlist_item_parent_get(event_info))
      return;

   Tree_Item *treeit = elm_object_item_data_get(event_info);

   Evas_Object *obj = treeit->ptr;
//   libclouseau_highlight(obj);

   clouseau_obj_information_list_populate(treeit);
}

static void
_set_selected_app(void *data, Evas_Object *pobj,
      void *event_info)
{  /* Set hovel label */
   elm_object_text_set(pobj, data);
}

static Ecore_Ipc_Server *
_connect_to_daemon(Evas_Object *gl)
{
   static Ecore_Ipc_Server *svr = NULL;
   const char *address = LOCALHOST;

   if (svr && ecore_ipc_server_connected_get(svr))
     return svr;  /* Already connected */

   if (!(svr = ecore_ipc_server_connect(ECORE_IPC_REMOTE_SYSTEM, LOCALHOST, PORT, NULL)))
     {
        printf("could not connect to the server: %s, port %d.\n",
              address, PORT);
        return NULL;
     }

   ecore_ipc_server_data_size_max_set(svr, -1);

   /* set event handler for server connect */
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)_add, gl);
   /* set event handler for server disconnect */
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)_del, gl);
   /* set event handler for receiving server data */
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)_data, gl);

   return svr;
}

void
_item_tree_print_string(Tree_Item *parent)
{
   Tree_Item *treeit;
   Eina_List *l;

   EINA_LIST_FOREACH(parent->children, l, treeit)
     {
         _item_tree_print_string(treeit);
     }

   if (parent->name)
     printf("From EET: <%s>\n", parent->name);
}

static int
_load_list(Evas_Object *gl)
{
   Ecore_Ipc_Server *svr = _connect_to_daemon(gl);
   if (svr)
     {
        void *p;
        char *msg="Asking refresg from GUI";
        int size = 0;

        ack_st t = { msg };
        p = packet_compose(GUI_ACK, &t, sizeof(t), &size);
        if (p)
          {
             ecore_ipc_server_send(svr, 0,0,0,0,EINA_FALSE, p, size);
             ecore_ipc_server_flush(svr);
             free(p);
          }
     }

   return 0;
}

static void
_show_clippers_check_changed(void *data, Evas_Object *obj,
      void *event_info __UNUSED__)
{
   list_show_clippers = elm_check_state_get(obj);
   _load_list(data);
}

static void
_show_hidden_check_changed(void *data, Evas_Object *obj,
      void *event_info __UNUSED__)
{
   list_show_hidden = elm_check_state_get(obj);
   _load_list(data);
}

static void
_bt_clicked(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   elm_object_text_set(obj, "Reload");
   _load_list(data);
}

#ifndef ELM_LIB_QUICKLAUNCH
EAPI int
elm_main(int argc, char **argv)
{  /* Create Client Window */
   Evas_Object *win, *bg, *panes, *bx, *bt, *show_hidden_check,
               *show_clippers_check, *dd_list, *gl;

   win = elm_win_add(NULL, "client", ELM_WIN_BASIC);
   elm_win_autodel_set(win, EINA_TRUE);
   elm_win_title_set(win, "client");

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   /* Control buttons */
     {
        Evas_Object *hbx;

        hbx = elm_box_add(bx);
        evas_object_size_hint_align_set(hbx, 0.0, 0.5);
        elm_box_horizontal_set(hbx, EINA_TRUE);
        elm_box_pack_end(bx, hbx);
        elm_box_padding_set(hbx, 10, 0);
        evas_object_show(hbx);

        bt = elm_button_add(hbx);
        evas_object_size_hint_align_set(bt, 0.0, 0.3);
        elm_object_text_set(bt, "Load");
        elm_box_pack_end(hbx, bt);
        evas_object_show(bt);

        dd_list = elm_hoversel_add(win);
        elm_hoversel_hover_parent_set(dd_list, win);
        elm_object_text_set(dd_list, "SELECT APP");
        elm_hoversel_item_add(dd_list, "app1", NULL, ELM_ICON_NONE,
              _set_selected_app, "app1");
        elm_hoversel_item_add(dd_list, "app2", NULL, ELM_ICON_NONE,
              _set_selected_app, "app2");
        elm_hoversel_item_add(dd_list, "app3", NULL, ELM_ICON_NONE,
              _set_selected_app, "app3");

        evas_object_size_hint_align_set(dd_list, 0.0, 0.3);
        elm_box_pack_end(hbx, dd_list);
        evas_object_show(dd_list);

        show_hidden_check = elm_check_add(hbx);
        elm_object_text_set(show_hidden_check, "Show Hidden");
        elm_check_state_set(show_hidden_check, list_show_hidden);
        elm_box_pack_end(hbx, show_hidden_check);
        evas_object_show(show_hidden_check);

        show_clippers_check = elm_check_add(hbx);
        elm_object_text_set(show_clippers_check, "Show Clippers");
        elm_check_state_set(show_clippers_check, list_show_clippers);
        elm_box_pack_end(hbx, show_clippers_check);
        evas_object_show(show_clippers_check);
     }

   panes = elm_panes_add(win);
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(panes, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, panes);
   evas_object_show(panes);

   /* The main list */
     {
        gl = elm_genlist_add(panes);
        evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_object_part_content_set(panes, "left", gl);
        evas_object_show(gl);

        evas_object_smart_callback_add(bt, "clicked", _bt_clicked, gl);
        evas_object_smart_callback_add(show_hidden_check, "changed",
              _show_hidden_check_changed, gl);
        evas_object_smart_callback_add(show_clippers_check, "changed",
              _show_clippers_check_changed, gl);

        itc.item_style = "default";
        itc.func.text_get = item_text_get;
        itc.func.content_get = item_icon_get;
        itc.func.state_get = NULL;
        itc.func.del = NULL;

        evas_object_smart_callback_add(gl, "expand,request", gl_exp_req, gl);
        evas_object_smart_callback_add(gl, "contract,request", gl_con_req, gl);
        evas_object_smart_callback_add(gl, "expanded", gl_exp, gl);
        evas_object_smart_callback_add(gl, "contracted", gl_con, gl);
        evas_object_smart_callback_add(gl, "selected", _gl_selected, NULL);
     }

   /* Properties list */
     {
        Evas_Object *prop_list = NULL;
        prop_list = clouseau_obj_information_list_add(panes);
        evas_object_size_hint_align_set(prop_list, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(prop_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

        elm_object_part_content_set(panes, "right", prop_list);
        evas_object_show(prop_list);
     }

   evas_object_resize(win, 500, 500);
   evas_object_show(win);

   evas_object_smart_callback_add(win, "delete,request", client_win_del, NULL);

   eina_init();
   ecore_init();
   ecore_ipc_init();

   if(!_connect_to_daemon(gl))
     {
        printf("Failed to connect to server.\n");
        return 0;
     }

   elm_run();
   elm_shutdown();

   data_descriptors_shutdown();
   printf("Client cleanup.\n");
   return 0;
}
ELM_MAIN()
#endif
