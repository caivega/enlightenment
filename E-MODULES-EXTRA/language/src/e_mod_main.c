#include <e.h>
#include "e_mod_main.h"
#include "e_mod_parse.h"

/* Static functions
 * The static functions specific to the current code unit.
 */

/* GADCON */

static E_Gadcon_Client *_gc_init(
    E_Gadcon *gc, const char *name, const char *id, const char *style
);

static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient  (E_Gadcon_Client *gcc, E_Gadcon_Orient orient);

static const char *_gc_label (E_Gadcon_Client_Class *client_class);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__);

static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);

/* CONFIG */

static void _e_xkb_cfg_new (void);
static void _e_xkb_cfg_free(void);

static Eina_Bool _e_xkb_cfg_timer(void *data);

/* EVENTS */

static void _e_xkb_cb_mouse_down(
    void *data, Evas *evas, Evas_Object *obj, void *event
);

static void _e_xkb_cb_menu_post     (void *data, E_Menu *menu);
static void _e_xkb_cb_menu_configure(void *data, E_Menu *mn, E_Menu_Item *mi);

/* Static variables
 * The static variables specific to the current code unit.
 */

/* GADGET INSTANCE */

typedef struct _Instance 
{
    E_Gadcon_Client *gcc;

    Evas_Object *o_xkbswitch;
    Evas_Object *o_xkbflag;

    E_Menu *menu;
} Instance;

/* LIST OF INSTANCES */
static Eina_List *instances = NULL;

/* EET STRUCTURES */

static E_Config_DD *e_xkb_cfg_edd        = NULL;
static E_Config_DD *e_xkb_cfg_layout_edd = NULL;

/* Global variables
 * Global variables shared across the module.
 */

/* CONFIG STRUCTURE */
E_XKB_Config *e_xkb_cfg = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
    GADCON_CLIENT_CLASS_VERSION, "xkbswitch", 
    {
        _gc_init,
        _gc_shutdown,
        _gc_orient,
        _gc_label,
        _gc_icon,
        _gc_id_new,
        NULL, NULL
    },
    E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "XKB Switcher" };

/* Module initializer
 * Initializes the configuration file, checks its versions, populates
 * menus, finds the rules file, initializes gadget icon.
 */
EAPI void *e_modapi_init(E_Module *m) 
{
    /* Locals */
    char buf[4096];

    /* i18n */

    snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));

    bindtextdomain         (PACKAGE, buf);
    bind_textdomain_codeset(PACKAGE, "UTF-8");

    /* Menus and dialogs */

    snprintf(buf, sizeof(buf), "%s/e-module-xkbswitch.edj", m->dir);

    e_configure_registry_category_add(
        "keyboard_and_mouse", 80, D_("Input"), 
        NULL, "preferences-behavior"
    );
    e_configure_registry_item_add(
        "keyboard_and_mouse/xkbswitch",
        110, D_("XKB Switcher"), 
        NULL, buf, e_xkb_cfg_dialog
    );

    /* Eet */

    e_xkb_cfg_layout_edd = E_CONFIG_DD_NEW(
        "E_XKB_Config_Layout", E_XKB_Config_Layout
    );
    #undef T
    #undef D
    #define T E_XKB_Config_Layout
    #define D e_xkb_cfg_layout_edd
    E_CONFIG_VAL(D, T, name,    STR);
    E_CONFIG_VAL(D, T, model,   STR);
    E_CONFIG_VAL(D, T, variant, STR);

    e_xkb_cfg_edd = E_CONFIG_DD_NEW(
        "e_xkb_cfg", E_XKB_Config
    );
    #undef T
    #undef D
    #define T E_XKB_Config
    #define D e_xkb_cfg_edd
    E_CONFIG_VAL(D, T, version, INT);
    E_CONFIG_LIST(D, T, used_layouts, e_xkb_cfg_layout_edd);

    /* Version check */

    e_xkb_cfg = e_config_domain_load("module.xkbswitch", e_xkb_cfg_edd);
    if (e_xkb_cfg) 
    {
        /* Check config version */
        if ((e_xkb_cfg->version >> 16) < MOD_CONFIG_FILE_EPOCH) 
        {
            /* config too old */
            _e_xkb_cfg_free();
            ecore_timer_add(1.0, _e_xkb_cfg_timer,
                 D_("XKB Switcher Module Configuration data needed "
                 "upgrading. Your old configuration<br> has been"
                 " wiped and a new set of defaults initialized. "
                 "This<br>will happen regularly during "
                 "development, so don't report a<br>bug. "
                 "This simply means the module needs "
                 "new configuration<br>data by default for "
                 "usable functionality that your old<br>"
                 "configuration simply lacks. This new set of "
                 "defaults will fix<br>that by adding it in. "
                 "You can re-configure things now to your<br>"
                 "liking. Sorry for the inconvenience.<br>")
            );
        }

        /* Ardvarks */
        else if (e_xkb_cfg->version > MOD_CONFIG_FILE_VERSION) 
        {
            /* config too new...wtf ? */
            _e_xkb_cfg_free();
            ecore_timer_add(1.0, _e_xkb_cfg_timer, 
                D_("Your XKB Switcher Module configuration is NEWER "
                "than the module version. This is "
                "very<br>strange. This should not happen unless"
                " you downgraded<br>the module or "
                "copied the configuration from a place where"
                "<br>a newer version of the module "
                "was running. This is bad and<br>as a "
                "precaution your configuration has been now "
                "restored to<br>defaults. Sorry for the "
                "inconvenience.<br>")
            );
        }
    }


    if (!e_xkb_cfg) _e_xkb_cfg_new();
    e_xkb_cfg->module = m;

    /* Gadcon */
    e_gadcon_provider_register(&_gc_class);

    /* Rules */
    find_rules();

    /* Update the icon & layout */
    e_xkb_update_icon  ();
    e_xkb_update_layout();

    return m;
}

/* Module shutdown
 * Called when the module gets unloaded. Deregisters the menu state
 * and frees up the config.
 */
EAPI int e_modapi_shutdown(E_Module *m) 
{
    E_XKB_Config_Layout *cl = NULL;

    e_configure_registry_item_del    ("keyboard_and_mouse/xkbswitch");
    e_configure_registry_category_del("keyboard_and_mouse");

    if (e_xkb_cfg->cfd)
        e_object_del(E_OBJECT(e_xkb_cfg->cfd));

    e_xkb_cfg->cfd    = NULL;
    e_xkb_cfg->module = NULL;

    e_gadcon_provider_unregister(&_gc_class);

    EINA_LIST_FREE(e_xkb_cfg->used_layouts, cl)
    {
        if (cl->name)    eina_stringshare_del(cl->name);
        if (cl->model)   eina_stringshare_del(cl->model);
        if (cl->variant) eina_stringshare_del(cl->variant);
    }

    E_FREE(e_xkb_cfg);

    E_CONFIG_DD_FREE(e_xkb_cfg_layout_edd);
    E_CONFIG_DD_FREE(e_xkb_cfg_edd);

    clear_rules();

    return 1;
}

/* Module state save
 * Used to save the configuration file on module shutdown.
 */
EAPI int e_modapi_save(E_Module *m) 
{
    e_config_domain_save("module.xkbswitch", e_xkb_cfg_edd, e_xkb_cfg);
    return 1;
}

/* Updates icons on all available xkbswitch gadgets to reflect the
 * current layout state.
 */
void e_xkb_update_icon(void)
{
    Instance  *inst = NULL;
    Eina_List *l    = NULL;
    char buf[4096];

    if (!e_xkb_cfg->used_layouts)
        return;

    const char *name = ((E_XKB_Config_Layout*)eina_list_data_get(
        e_xkb_cfg->used_layouts
    ))->name;

    snprintf(
        buf, sizeof(buf), "%s/flags/%s_flag.png",
        e_module_dir_get(e_xkb_cfg->module), name
    );

    EINA_LIST_FOREACH(instances, l, inst)
    {
        evas_object_hide          (inst->o_xkbflag);
        edje_object_part_unswallow(inst->o_xkbswitch, inst->o_xkbflag);

        e_icon_file_set(inst->o_xkbflag, buf);

        edje_object_part_swallow (inst->o_xkbswitch, "flag",  inst->o_xkbflag);
        edje_object_part_text_set(inst->o_xkbswitch, "label", name);

        evas_object_show(inst->o_xkbflag);
    }
}

void e_xkb_update_layout(void)
{
    E_XKB_Config_Layout *cl = NULL;
    char buf[256];

    if (!e_xkb_cfg->used_layouts)
        return;

    cl = eina_list_data_get(e_xkb_cfg->used_layouts);

    snprintf(
        buf, sizeof(buf), "setxkbmap -layout %s -variant %s -model %s",
        cl->name, cl->variant, cl->model
    );
    printf("executing %s\n", buf);
    ecore_exe_run(buf, NULL);
}

/* LOCAL STATIC FUNCTIONS */

static E_Gadcon_Client *_gc_init(
    E_Gadcon *gc, const char *name, const char *id, const char *style
) 
{
    Instance *inst = NULL;
    char buf[4096];

    snprintf(
        buf, sizeof(buf), "%s/e-module-xkbswitch.edj", 
        e_xkb_cfg->module->dir
    );

    /* The instance */
    inst = E_NEW(Instance, 1);

    /* The gadget */
    inst->o_xkbswitch = edje_object_add(gc->evas);
    if (!e_theme_edje_object_set(
        inst->o_xkbswitch, "base/theme/modules/xkbswitch", 
        "modules/xkbswitch/main"
    )) edje_object_file_set(inst->o_xkbswitch, buf, "modules/xkbswitch/main");

    /* The gadcon client */
    inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_xkbswitch);
    inst->gcc->data = inst;

    /* The flag icon */
    inst->o_xkbflag = e_icon_add(gc->evas);
    snprintf(
        buf, sizeof(buf), "%s/flags/unknown_flag.png",
        e_module_dir_get(e_xkb_cfg->module)
    );
    e_icon_file_set(inst->o_xkbflag, buf);

    /* The icon is part of the gadget. */
    edje_object_part_swallow(inst->o_xkbswitch, "flag", inst->o_xkbflag);

    /* Hook some menus */
    evas_object_event_callback_add(
        inst->o_xkbswitch, EVAS_CALLBACK_MOUSE_DOWN,
        _e_xkb_cb_mouse_down, inst
    );

    /* Make the list know about the instance */
    instances = eina_list_append(instances, inst);

    return inst->gcc;
}

static void _gc_shutdown(E_Gadcon_Client *gcc) 
{
    Instance *inst = NULL;

    if (!(inst = gcc->data)) return;
    instances = eina_list_remove(instances, inst);

    if (inst->menu) 
    {
        e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
    }

    if (inst->o_xkbswitch) 
    {
        evas_object_event_callback_del(
            inst->o_xkbswitch,
            EVAS_CALLBACK_MOUSE_DOWN, 
            _e_xkb_cb_mouse_down
        );
        evas_object_del(inst->o_xkbswitch);
        evas_object_del(inst->o_xkbflag);
    }

    E_FREE(inst);
}

static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient) 
{
    e_gadcon_client_aspect_set  (gcc, 16, 16);
    e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *_gc_label(E_Gadcon_Client_Class *client_class)
{
    return D_("XKB Switcher");
}

static const char *_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__) 
{
    return _gc_class.name;
}

static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas) 
{
    Evas_Object *o = NULL;
    char buf[4096];

    snprintf(
        buf, sizeof(buf), "%s/e-module-xkbswitch.edj",
        e_xkb_cfg->module->dir
    );

    o = edje_object_add(evas);
    edje_object_file_set(o, buf, "icon");

    return o;
}

static void _e_xkb_cfg_new(void) 
{
    char buf[128];

    e_xkb_cfg = E_NEW(E_XKB_Config, 1);
    e_xkb_cfg->version = (MOD_CONFIG_FILE_EPOCH << 16);

    if ((e_xkb_cfg->version & 0xffff) < 0x008d)
    {
        e_xkb_cfg->used_layouts = NULL;
    }

    e_xkb_cfg->version = MOD_CONFIG_FILE_VERSION;

    e_config_save_queue();
}

static void _e_xkb_cfg_free(void) 
{
    E_XKB_Config_Layout *cl = NULL;

    EINA_LIST_FREE(e_xkb_cfg->used_layouts, cl)
    {
        if (cl->name)    eina_stringshare_del(cl->name);
        if (cl->model)   eina_stringshare_del(cl->model);
        if (cl->variant) eina_stringshare_del(cl->variant);
    }

    E_FREE(e_xkb_cfg);
}

static Eina_Bool _e_xkb_cfg_timer(void *data) 
{
    e_util_dialog_internal( D_("XKB Switcher Configuration Updated"), data);
    return EINA_FALSE;
}

static void _e_xkb_cb_mouse_down(
    void *data, Evas *evas, Evas_Object *obj, void *event
) 
{
    Evas_Event_Mouse_Down *ev = NULL;
    E_Menu_Item           *mi = NULL;

    Instance *inst = NULL;
    E_Zone   *zone = NULL;

    int x, y;

    if (!(inst = data))
        return;

    ev = event;

    if ((ev->button == 3) && (!inst->menu)) 
    {
        E_Menu *m = NULL;

        zone = e_util_zone_current_get(e_manager_current_get());

        m  = e_menu_new();
        mi = e_menu_item_new(m);

        e_menu_item_label_set          (mi, D_("Settings"));
        e_util_menu_item_theme_icon_set(mi, "preferences-system");
        e_menu_item_callback_set       (mi, _e_xkb_cb_menu_configure, NULL);

        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
        e_menu_post_deactivate_callback_set(m, _e_xkb_cb_menu_post, inst);
        inst->menu = m;

        e_gadcon_canvas_zone_geometry_get(
            inst->gcc->gadcon, &x, &y, 
            NULL, NULL
        );

        e_menu_activate_mouse(
            m, zone, (x + ev->output.x), 
            (y + ev->output.y), 1, 1, 
            E_MENU_POP_DIRECTION_AUTO, ev->timestamp
        );
        evas_event_feed_mouse_up(
            inst->gcc->gadcon->evas,
            ev->button, EVAS_BUTTON_NONE,
            ev->timestamp, NULL
        );
    }
    else if (ev->button == 1)
    {
        void *data = eina_list_data_get(e_xkb_cfg->used_layouts);

        e_xkb_cfg->used_layouts = eina_list_append(
            e_xkb_cfg->used_layouts, data
        );
        e_xkb_cfg->used_layouts = eina_list_remove_list(
            e_xkb_cfg->used_layouts, e_xkb_cfg->used_layouts
        );

        e_xkb_update_icon  ();
        e_xkb_update_layout();
    }
}

static void _e_xkb_cb_menu_post(void *data, E_Menu *menu) 
{
    Instance *inst = NULL;

    if (!(inst = data) || !inst->menu) return;
    inst->menu = NULL;
}

static void _e_xkb_cb_menu_configure(void *data, E_Menu *mn, E_Menu_Item *mi) 
{
    if (!e_xkb_cfg || e_xkb_cfg->cfd) return;
    e_xkb_cfg_dialog(mn->zone->container, NULL);
}
