# Copyright (c) 2008-2009 Simon Busch
#
# This file is part of python-elementary.
#
# python-elementary is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# python-elementary is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with python-elementary.  If not, see <http://www.gnu.org/licenses/>.
#

cdef void _toolbar_callback(void *cbt, Evas_Object *obj, void *event_info) with gil:
    try:
        (toolbar, callback, it, a, ka) = <object>cbt
        callback(toolbar, it, *a, **ka)
    except Exception, e:
        traceback.print_exc()

cdef void _toolbar_item_del_cb(void *data, Evas_Object *o, void *event_info) with gil:
    (obj, callback, it, a, ka) = <object>data
    it.__del_cb()

cdef class ToolbarItem(ObjectItem):

    """An item for the toolbar."""

    def __init__(self, evasObject toolbar, icon, label,
                 callback, *args, **kargs):
        cdef Evas_Object *ic = NULL
        cdef void* cbdata = NULL
        cdef void (*cb) (void *, Evas_Object *, void *)
        cb = NULL

        if callback:
            if not callable(callback):
                raise TypeError("callback is not callable")
            cb = _hoversel_callback

        self.cbt = (toolbar, callback, self, args, kargs)
        cbdata = <void*>self.cbt
        self.item = elm_toolbar_item_append(toolbar.obj, icon, _cfruni(label), cb, cbdata)

        Py_INCREF(self)
        elm_object_item_del_cb_set(self.item, _toolbar_item_del_cb)

    def next_get(self):
        """Get the item after C{item} in toolbar.

        @note: If it is the last item, C{None} will be returned.

        @see: L{Toolbar.item_append()}

        @return: The item after C{item}, or C{None} if none or on failure.
        @rtype: L{ToolbarItem}

        """
        cdef Elm_Object_Item *it
        it = elm_toolbar_item_next_get(self.item)
        return _object_item_to_python(it)

    property next:
        def __get__(self):
            return self.next_get()

    def prev_get(self):
        """Get the item before C{item} in toolbar.

        @note: If it is the first item, C{None} will be returned.

        @see: L{Toolbar.item_prepend()}

        @return: The item before C{item}, or C{None} if none or on failure.
        @rtype: L{ToolbarItem}

        """
        cdef Elm_Object_Item *it
        it = elm_toolbar_item_prev_get(self.item)
        return _object_item_to_python(it)

    property prev:
        def __get__(self):
            return self.prev_get()

    def priority_set(self, priority):
        """Set the priority of a toolbar item.

        This is used only when the toolbar shrink mode is set to
        ELM_TOOLBAR_SHRINK_MENU or ELM_TOOLBAR_SHRINK_HIDE. When space is
        less than required, items with low priority will be removed from the
        toolbar and added to a dynamically-created menu, while items with
        higher priority will remain on the toolbar, with the same order they
        were added.

        @see: L{priority_get()}

        @param priority: The item priority. The default is zero.
        @type priority: int

        """
        elm_toolbar_item_priority_set(self.item, priority)

    def priority_get(self):
        """Get the priority of a toolbar item.

        @see: L{priority_set()} for details.

        @return: The item priority, or C{0} on failure.
        @rtype: int

        """
        return elm_toolbar_item_priority_get(self.item)

    property priority:
        def __get__(self):
            return self.priority_get()

        def __set__(self, priority):
            self.priority_set(priority)

    def selected_get(self):
        """Get whether the item is selected or not.

        @see: L{Toolbar.selected_item_get()} for details.
        @see: L{selected_get()}

        @return: C{True} means item is selected. C{False} indicates it's not.
        @rtype: bool

        """
        return elm_toolbar_item_selected_get(self.item)

    def selected_set(self, selected):
        """Set the selected state of an item.

        This sets the selected state of the given item.
        C{True} for selected, C{False} for not selected.

        If a new item is selected the previously selected will be unselected.
        Previously selected item can be get with function
        L{Toolbar.selected_item_get()}.

        Selected items will be highlighted.

        @see: L{selected_get()}
        @see: L{Toolbar.selected_item_get()}

        @param selected: The selected state
        @type selected: bool

        """
        elm_toolbar_item_selected_set(self.item, selected)

    property selected:
        def __set__(self, selected):
            elm_toolbar_item_selected_set(self.item, selected)

        def __get__(self):
            return elm_toolbar_item_selected_get(self.item)

    def icon_set(self, ic):
        """Set the icon associated with the item.

        Toolbar will load icon image from fdo or current theme.
        This behavior can be set by L{Toolbar.icon_order_lookup_set()} function.
        If an absolute path is provided it will load it direct from a file.

        @see: L{Toolbar.icon_order_lookup_set()}
        @see: L{Toolbar.icon_order_lookup_get()}

        @param icon: A string with icon name or the absolute path of an
            image file.
        @type icon: string

        """
        elm_toolbar_item_icon_set(self.item, _cfruni(ic))

    def icon_get(self):
        """Get the string used to set the icon of item.

        @see: L{icon_set()} for details.

        @return: The string associated with the icon object.
        @rtype: string

        """
        return _ctouni(elm_toolbar_item_icon_get(self.item))

    property icon:
        def __get__(self):
            return self.icon_get()

        def __set__(self, ic):
            self.icon_set(ic)

    def object_get(self):
        """Get the object of item.

        @return: The object
        @rtype: L{Object}

        """
        cdef Evas_Object *obj = elm_toolbar_item_object_get(self.item)
        return Object_from_instance(obj)

    def icon_object_get(self):
        """Get the icon object of item.

        @see: L{icon_set()}, L{icon_file_set()}, or L{icon_memfile_set()} for
            details.

        @return: The icon object
        @rtype: L{Icon}

        """
        cdef Evas_Object *obj = elm_toolbar_item_icon_object_get(self.item)
        return Object_from_instance(obj)

    #TODO def icon_memfile_set(self, img, size, format, key):
        """Set the icon associated with item to an image in a binary buffer.

        @note: The icon image set by this function can be changed by
            L{icon_set()}.

        @param img: The binary data that will be used as an image
        @param size: The size of binary data C{img}
        @type size: int
        @param format: Optional format of C{img} to pass to the image loader
        @type format: string
        @param key: Optional key of C{img to pass to the image loader (eg.
            if C{img is an edje file)
        @type key: string

        @return: (C{True = success, C{False = error)
        @rtype: bool

        """
        #return bool(elm_toolbar_item_icon_memfile_set(self.item, img, size, format, key))

    def icon_file_set(self, file, key):
        """Set the icon associated with item to an image in a binary buffer.

        @note: The icon image set by this function can be changed by
            L{icon_set()}.

        @param file: The file that contains the image
        @type file: string
        @param key: Optional key of C{img} to pass to the image loader (eg.
            if C{img} is an edje file)
        @type key: string

        @return: (C{True} = success, C{False} = error)
        @rtype: bool

        """
        return bool(elm_toolbar_item_icon_file_set(self.item, _cfruni(file), _cfruni(key)))

    def separator_set(self, separator):
        """Set or unset item as a separator.

        Items aren't set as separator by default.

        If set as separator it will display separator theme, so won't display
        icons or label.

        @see: L{separator_get()}

        @param separator: C{True} to set item item as separator or C{False}
            to unset, i.e., item will be used as a regular item.
        @type separator: bool

        """
        elm_toolbar_item_separator_set(self.item, separator)

    def separator_get(self):
        """Get a value whether item is a separator or not.

        @see: L{separator_set()} for details.

        @return: C{True} means item is a separator. C{False} indicates it's
            not.
        @rtype: bool

        """
        return elm_toolbar_item_separator_get(self.item)

    property separator:
        def __set__(self, separator):
            elm_toolbar_item_separator_set(self.item, separator)

        def __get__(self):
            return elm_toolbar_item_separator_get(self.item)

    def menu_set(self, menu):
        """Set whether the toolbar item opens a menu.

        A toolbar item can be set to be a menu, using this function.

        Once it is set to be a menu, it can be manipulated through the
        menu-like function L{menu_parent_set()} and the other menu
        functions, using the Evas_Object C{menu} returned by
        L{item_menu_get()}.

        So, items to be displayed in this item's menu should be added with
        L{Menu.item_add()}.

        The following code exemplifies the most basic usage::
            tb = Toolbar(win)
            item = tb.item_append("refresh", "Menu")
            item.menu_set(True)
            tb.menu_parent_set(win)
            menu = item.menu_get()
            menu.item_add(None, "edit-cut", "Cut")
            menu_item = menu.item_add(None, "edit-copy", "Copy")

        @see: L{menu_get()}

        @param menu: If C{True}, item will opens a menu when selected.
        @type menu: bool

        """
        elm_toolbar_item_menu_set(self.item, menu)

    def menu_get(self):
        """Get toolbar item's menu.

        If item wasn't set as menu item with L{menu_set()}, this function
        will set it.

        @see: L{menu_set()} for details.

        @return: Item's menu object or C{None} on failure.

        """
        cdef Evas_Object *menu
        menu = elm_toolbar_item_menu_get(self.item)
        if menu == NULL:
            return None
        else:
            return Menu(None, <object>menu)

    property menu:
        def __get__(self):
            return self.menu_get()

        def __set__(self, value):
            self.menu_set(value)


    #TODO def state_add(self, icon, label, func, data):
        #return elm_toolbar_item_state_add(self.item, icon, label, func, data)

    #TODO def state_del(self, state):
        #return bool(elm_toolbar_item_state_del(self.item, state))

    #TODO def state_set(self, state):
        #return bool(elm_toolbar_item_state_set(self.item, state))

    #TODO def state_unset(self):
        #elm_toolbar_item_state_unset(self.item)

    #TODO def state_get(self):
        #return elm_toolbar_item_state_get(self.item)

    #TODO def state_next(self):
        #return elm_toolbar_item_state_next(self.item)

    #TODO def state_prev(self):
        #return elm_toolbar_item_state_prev(self.item)

cdef public class Toolbar(Object) [object PyElementaryToolbar, type PyElementaryToolbar_Type]:

    """A toolbar is a widget that displays a list of items inside a box. It
    can be scrollable, show a menu with items that don't fit to toolbar size
    or even crop them.

    Only one item can be selected at a time.

    Items can have multiple states, or show menus when selected by the user.

    Smart callbacks one can listen to:
        - "clicked" - when the user clicks on a toolbar item and becomes selected.
        - "longpressed" - when the toolbar is pressed for a certain amount of time.
        - "language,changed" - when the program language changes.

    Available styles for it:
        - C{"default"}
        - C{"transparent"} - no background or shadow, just show the content

    Default text parts of the toolbar items that you can use for are:
        - "default" - label of the toolbar item

    """

    def __init__(self, evasObject parent):
        Object.__init__(self, parent.evas)
        self._set_obj(elm_toolbar_add(parent.obj))

    def icon_size_set(self, icon_size):
        """Set the icon size, in pixels, to be used by toolbar items.

        @note: Default value is C{32}. It reads value from elm config.

        @see: L{icon_size_get()}

        @param icon_size: The icon size in pixels
        @type icon_size: int

        """
        elm_toolbar_icon_size_set(self.obj, icon_size)

    def icon_size_get(self):
        """Get the icon size, in pixels, to be used by toolbar items.

        @see: L{icon_size_set()} for details.

        @return: The icon size in pixels.
        @rtype: int

        """
        return elm_toolbar_icon_size_get(self.obj)

    property icon_size:
        def __set__(self, icon_size):
            elm_toolbar_icon_size_set(self.obj, icon_size)

        def __get__(self):
            return elm_toolbar_icon_size_get(self.obj)

    def icon_order_lookup_set(self, order):
        """Sets icon lookup order, for toolbar items' icons.

        Icons added before calling this function will not be affected.
        The default lookup order is ELM_ICON_LOOKUP_THEME_FDO.

        @see: L{icon_order_lookup_get()}

        @param order: The icon lookup order.
        @type order: Elm_Icon_Lookup_Order

        """
        elm_toolbar_icon_order_lookup_set(self.obj, order)

    def icon_order_lookup_get(self):
        """Gets the icon lookup order.

        @see: L{icon_order_lookup_set()} for details.

        @return: The icon lookup order.
        @rtype: Elm_Icon_Lookup_Order

        """
        return elm_toolbar_icon_order_lookup_get(self.obj)

    property icon_order_lookup:
        def __set__(self, order):
            elm_toolbar_icon_order_lookup_set(self.obj, order)

        def __get__(self):
            return elm_toolbar_icon_order_lookup_get(self.obj)

    def item_append(self, icon, label, callback = None, *args, **kargs):
        """Append item to the toolbar.

        A new item will be created and appended to the toolbar, i.e., will
        be set as B{last} item.

        Items created with this method can be deleted with
        L{ObjectItem.delete()}.

        If a function is passed as argument, it will be called every time
        this item is selected, i.e., the user clicks over an unselected item.
        If such function isn't needed, just passing C{None} as C{func} is
        enough. The same should be done for C{data}.

        Toolbar will load icon image from fdo or current theme. This
        behavior can be set by L{icon_order_lookup_set()} function.
        If an absolute path is provided it will load it direct from a file.

        @see: L{ToolbarItem.icon_set()}
        @see: L{ObjectItem.delete()}

        @param icon: A string with icon name or the absolute path of an image file.
        @type icon: string
        @param label: The label of the item.
        @type label: string
        @param func: The function to call when the item is clicked.
        @type func: function
        @return: The created item or C{None} upon failure.
        @rtype: ToolbarItem

        """
        # Everything is done in the ToolbarItem class, because of wrapping the
        # C structures in python classes
        return ToolbarItem(self, icon, label, callback, *args, **kargs)

    #TODO: def item_prepend(self, icon, label, callback = None, *args, **kargs):
        """Prepend item to the toolbar.

        A new item will be created and prepended to the toolbar, i.e., will
        be set as B{first} item.

        Items created with this method can be deleted with
        L{ObjectItem.delete()}.

        If a function is passed as argument, it will be called every time
        this item is selected, i.e., the user clicks over an unselected item.
        If such function isn't needed, just passing C{None} as C{func} is
        enough. The same should be done for C{data}.

        Toolbar will load icon image from fdo or current theme. This
        behavior can be set by L{icon_order_lookup_set()} function.
        If an absolute path is provided it will load it direct from a file.

        @see: L{ToolbarItem.icon_set()}
        @see: L{ObjectItem.delete()}

        @param icon: A string with icon name or the absolute path of an image file.
        @type icon: string
        @param label: The label of the item.
        @type label: string
        @param func: The function to call when the item is clicked.
        @type func: function
        @return: The created item or C{None} upon failure.
        @rtype: L{ToolbarItem}

        """
        #return ToolbarItem(self, icon, label, callback, *args, **kargs)

    #TODO: def item_insert_before(self, before, icon, label, callback = None, *args, **kargs):
        """Insert a new item into the toolbar object before item C{before}.

        A new item will be created and added to the toolbar. Its position in
        this toolbar will be just before item C{before}.

        Items created with this method can be deleted with
        L{ObjectItem.delete()}.

        If a function is passed as argument, it will be called every time
        this item is selected, i.e., the user clicks over an unselected item.
        If such function isn't needed, just passing C{None} as C{func} is
        enough. The same should be done for C{data}.

        Toolbar will load icon image from fdo or current theme. This
        behavior can be set by L{icon_order_lookup_set()} function.
        If an absolute path is provided it will load it direct from a file.

        @see: L{ToolbarItem.icon_set()}
        @see: L{ObjectItem.delete()}

        @param before: The toolbar item to insert before.
        @type before: L{ToolbarItem}
        @param icon: A string with icon name or the absolute path of an image file.
        @type icon: string
        @param label: The label of the item.
        @type label: string
        @param func: The function to call when the item is clicked.
        @type func: function
        @return: The created item or C{None} upon failure.
        @rtype: L{ToolbarItem}

        """
        #return ToolbarItem(self, icon, label, callback, *args, **kargs)

    #TODO: def item_insert_after(self, after, icon, label, callback = None, *args, **kargs):
        """Insert a new item into the toolbar object after item C{after}.

        A new item will be created and added to the toolbar. Its position in
        this toolbar will be just after item C{after}.

        Items created with this method can be deleted with
        L{ObjectItem.delete()}.

        If a function is passed as argument, it will be called every time
        this item is selected, i.e., the user clicks over an unselected item.
        If such function isn't needed, just passing C{None} as C{func} is
        enough. The same should be done for C{data}.

        Toolbar will load icon image from fdo or current theme. This
        behavior can be set by L{icon_order_lookup_set()} function.
        If an absolute path is provided it will load it direct from a file.

        @see: L{ToolbarItem.icon_set()}
        @see: L{ObjectItem.delete()}

        @param after: The toolbar item to insert after.
        @type after: L{ToolbarItem}
        @param icon: A string with icon name or the absolute path of an image file.
        @type icon: string
        @param label: The label of the item.
        @type label: string
        @param func: The function to call when the item is clicked.
        @type func: function
        @return: The created item or C{None} upon failure.
        @rtype: L{ToolbarItem}

        """
        #return ToolbarItem(self, icon, label, callback, *args, **kargs)

    def first_item_get(self):
        """Get the first item in the given toolbar widget's list of items.

        @see: L{item_append()}
        @see: L{last_item_get()}

        @return: The first item or C{None}, if it has no items (and on errors)
        @rtype: L{ToolbarItem}

        """
        cdef Elm_Object_Item *it
        it = elm_toolbar_first_item_get(self.obj)
        return _object_item_to_python(it)

    property first_item:
        def __get__(self):
            return self.first_item_get()

    def last_item_get(self):
        """Get the last item in the given toolbar widget's list of items.

        @see: L{item_prepend()}
        @see: L{first_item_get()}

        @return: The last item or C{None}, if it has no items (and on errors)
        @rtype: L{ToolbarItem}

        """
        cdef Elm_Object_Item *it
        it = elm_toolbar_last_item_get(self.obj)
        return _object_item_to_python(it)

    property last_item:
        def __get__(self):
            return self.last_item_get()

    def elm_toolbar_item_find_by_label(self, label):
        """Returns a toolbar item by its label.

        @param label: The label of the item to find.
        @type label: string
        @return: The toolbar item matching C{label} or C{None} on failure.
        @rtype: L{ToolbarItem}

        """
        cdef Elm_Object_Item *it = elm_toolbar_item_find_by_label(self.obj, _cfruni(label))
        return _object_item_to_python(it)

    def selected_item_get(self):
        """Get the selected item.

        The selected item can be unselected with function
        elm_toolbar_item_selected_set().

        The selected item always will be highlighted on toolbar.

        @see: L{selected_items_get()}

        @return: The selected toolbar item.
        @rtype: L{ToolbarItem}

        """
        cdef Elm_Object_Item *it
        it = elm_toolbar_selected_item_get(self.obj)
        return _object_item_to_python(it)

    property selected_item:
        def __get__(self):
            return self.selected_item_get()

    def more_item_get(self):
        """Get the more item.

        The more item can be changed with function
        L{ObjectItem.text_set()} and L{ObjectItem.content_set()}.

        @return: The toolbar more item.
        @rtype: L{ToolbarItem}

        """
        cdef Elm_Object_Item *it
        it = elm_toolbar_more_item_get(self.obj)
        return _object_item_to_python(it)

    def shrink_mode_set(self, mode):
        """Set the shrink state of toolbar.

        The toolbar won't scroll if ELM_TOOLBAR_SHRINK_NONE, but will
        enforce a minimum size so all the items will fit, won't scroll and
        won't show the items that don't fit if ELM_TOOLBAR_SHRINK_HIDE, will
        scroll if ELM_TOOLBAR_SHRINK_SCROLL, and will create a button to pop
        up excess elements with ELM_TOOLBAR_SHRINK_MENU.

        @param shrink_mode: Toolbar's items display behavior.
        @type shrink_mode: Elm_Toolbar_Shrink_Mode

        """
        elm_toolbar_shrink_mode_set(self.obj, mode)

    def shrink_mode_get(self):
        """Get the shrink mode of toolbar.

        @see: L{shrink_mode_set()} for details.

        @return: Toolbar's items display behavior.
        @rtype: Elm_Toolbar_Shrink_Mode

        """
        return elm_toolbar_shrink_mode_get(self.obj)

    property shrink_mode:
        def __get__(self):
            return self.shrink_mode_get()

        def __set__(self, value):
            self.shrink_mode_set(value)

    def homogeneous_set(self, homogeneous):
        """Enable/disable homogeneous mode.

        This will enable the homogeneous mode where items are of the same size.

        @see: L{homogeneous_get()}

        @param homogeneous: Assume the items within the toolbar are of the
            same size (True = on, False = off). Default is C{False}.
        @type homogeneous: bool

        """
        elm_toolbar_homogeneous_set(self.obj, homogeneous)

    def homogeneous_get(self):
        """Get whether the homogeneous mode is enabled.

        @see: L{homogeneous_set()}

        @return: Assume the items within the toolbar are of the same height
            and width (True = on, False = off).
        @rtype: bool

        """
        return elm_toolbar_homogeneous_get(self.obj)

    property homogeneous:
        def __set__(self, homogeneous):
            elm_toolbar_homogeneous_set(self.obj, homogeneous)

        def __get__(self):
            return elm_toolbar_homogeneous_get(self.obj)

    def menu_parent_set(self, evasObject parent):
        """Set the parent object of the toolbar items' menus.

        Each item can be set as item menu, with L{ToolbarItem.menu_set()}.

        For more details about setting the parent for toolbar menus, see
        L{Menu.parent_set()}.

        @see: L{Menu.parent_set()} for details.
        @see: L{ToolbarItem.menu_set()} for details.

        @param parent: The parent of the menu objects.
        @type parent: L{Object}

        """
        elm_toolbar_menu_parent_set(self.obj, parent.obj)

    def menu_parent_get(self):
        """Get the parent object of the toolbar items' menus.

        @see: L{Menu.parent_set()} for details.

        @return: The parent of the menu objects.
        @rtype: L{Object}

        """
        cdef Evas_Object *parent = elm_toolbar_menu_parent_get(self.obj)
        return Object_from_instance(parent)

    property menu_parent:
        def __get__(self):
            return self.menu_parent_get()

        def __set__(self, value):
            self.menu_parent_set(value)

    def align_set(self, align):
        """Set the alignment of the items.

        Alignment of toolbar items, from C{0.0} to indicates to align
        left, to C{1.0}, to align to right. C{0.5} centralize
        items.

        Centered items by default.

        @see: L{align_get()}

        @param align: The new alignment, a float between C{0.0} and C{1.0}.
        @type align: float

        """
        elm_toolbar_align_set(self.obj, align)

    def align_get(self):
        """Get the alignment of the items.

        @see: L{align_set()} for details.

        @return: toolbar items alignment, a float between C{0.0} and C{1.0}.
        @rtype: float

        """
        return elm_toolbar_align_get(self.obj)

    property align:
        def __set__(self, align):
            elm_toolbar_align_set(self.obj, align)

        def __get__(self):
            return elm_toolbar_align_get(self.obj)

    def horizontal_set(self, horizontal):
        """Change a toolbar's orientation

        By default, a toolbar will be horizontal. Use this function to
        create a vertical toolbar.

        @param horizontal: If C{True}, the toolbar is horizontal
        @type horizontal: bool

        """
        elm_toolbar_horizontal_set(self.obj, horizontal)

    def horizontal_get(self):
        """Get a toolbar's orientation

        By default, a toolbar will be horizontal. Use this function to
        determine whether a toolbar is vertical.

        @return: If C{True}, the toolbar is horizontal
        @rtype: bool

        """
        return elm_toolbar_horizontal_get(self.obj)

    property horizontal:
        def __set__(self, horizontal):
            elm_toolbar_horizontal_set(self.obj, horizontal)

        def __get__(self):
            return elm_toolbar_horizontal_get(self.obj)

    def items_count(self):
        """Get the number of items in a toolbar

        @return: The number of items in toolbar
        @rtype: int

        """
        return elm_toolbar_items_count(self.obj)

    property standard_priority:
        """The standard priority of visible items in a toolbar

        If the priority of the item is up to standard priority, it is shown
        in basic panel. The other items are located in more menu or panel.
        The more menu or panel can be shown when the more item is clicked.

        @type: int

        """
        def __set__(self, priority):
            elm_toolbar_standard_priority_set(self.obj, priority)
        def __get__(self):
            return elm_toolbar_standard_priority_get(self.obj)

    def select_mode_set(self, mode):
        """Set the toolbar select mode.

        This function changes the item select mode in the toolbar widget.
            - ELM_OBJECT_SELECT_MODE_DEFAULT : Items will only call their
              selection func and callback when first becoming selected. Any
              further clicks will do nothing, unless you set always select
              mode.
            - ELM_OBJECT_SELECT_MODE_ALWAYS :  This means that, even if
              selected, every click will make the selected callbacks be called.
            - ELM_OBJECT_SELECT_MODE_NONE : This will turn off the ability
              to select items entirely and they will neither appear selected
              nor call selected callback functions.

        @see: L{select_mode_get()}

        @param mode: The select mode
        @type mode: Elm_Object_Select_Mode

        """
        elm_toolbar_select_mode_set(self.obj, mode)

    def select_mode_get(self):
        """Get the toolbar select mode.

        @see: L{select_mode_set()}

        @return: The select mode (ELM_OBJECT_SELECT_MODE_MAX on failure)
        @rtype: Elm_Object_Select_Mode

        """
        return elm_toolbar_select_mode_get(self.obj)

    property select_mode:
        def __get__(self):
            return self.select_mode_get()

        def __set__(self, value):
            self.select_mode_set(value)

    def callback_clicked_add(self, func, *args, **kwargs):
        """When the user clicks on a toolbar item and becomes selected."""
        self._callback_add("clicked", func, *args, **kwargs)

    def callback_clicked_del(self, func):
        self._callback_del("clicked", func)

    def callback_longpressed_add(self, func, *args, **kwargs):
        """When the toolbar is pressed for a certain amount of time."""
        self._callback_add("longpressed", func, *args, **kwargs)

    def callback_longpressed_del(self, func):
        self._callback_del("longpressed", func)

    def callback_language_changed_add(self, func, *args, **kwargs):
        """When the program language changes."""
        self._callback_add("language,changed", func, *args, **kwargs)

    def callback_language_changed_del(self, func):
        self._callback_del("language,changed", func)

_elm_widget_type_register("toolbar", Toolbar)

cdef extern from "Elementary.h": # hack to force type to be known
    cdef PyTypeObject PyElementaryToolbar_Type # hack to install metaclass
_install_metaclass(&PyElementaryToolbar_Type, ElementaryObjectMeta)
