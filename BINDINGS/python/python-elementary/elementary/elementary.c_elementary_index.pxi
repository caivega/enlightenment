# Copyright 2012 Kai Huuhko <kai.huuhko@gmail.com>
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

cdef void _index_callback(void *cbt, c_evas.Evas_Object *o, void *event_info) with gil:
    try:
        (obj, callback, it, a, ka) = <object>cbt
        callback(obj, it, *a, **ka)
    except Exception, e:
        traceback.print_exc()

cdef void _index_item_del_cb(void *data, c_evas.Evas_Object *o, void *event_info) with gil:
    (obj, callback, it, a, ka) = <object>data
    it.__del_cb()

def _index_item_conv(long addr):
    cdef Elm_Object_Item *it = <Elm_Object_Item *>addr
    cdef void *data = elm_object_item_data_get(it)
    if data == NULL:
        return None
    else:
        cbt = <object>data
        return cbt[2]

cdef enum Elm_Index_Item_Insert_Kind:
    ELM_INDEX_ITEM_INSERT_APPEND
    ELM_INDEX_ITEM_INSERT_PREPEND
    ELM_INDEX_ITEM_INSERT_BEFORE
    ELM_INDEX_ITEM_INSERT_AFTER
    ELM_INDEX_ITEM_INSERT_SORTED

cdef class IndexItem(ObjectItem):
    def __init__(self, kind, c_evas.Object index, letter, IndexItem before_after = None,
                 callback = None, *args, **kargs):
        cdef void* cbdata
        cdef void (*cb) (void *, c_evas.Evas_Object *, void *)

        cbdata = NULL
        cb = NULL

        if callback is not None:
            if not callable(callback):
                raise TypeError("callback is not callable")
            cb = _index_callback
        self.cbt = (index, callback, self, args, kargs)
        cbdata = <void*>self.cbt

        if kind == ELM_INDEX_ITEM_INSERT_APPEND:
            self.item = elm_index_item_append(index.obj, _cfruni(letter), cb, cbdata)
        elif kind == ELM_INDEX_ITEM_INSERT_PREPEND:
            self.item = elm_index_item_prepend(index.obj, _cfruni(letter), cb, cbdata)
        #elif kind == ELM_INDEX_ITEM_INSERT_SORTED:
            #self.item = elm_index_item_sorted_insert(index.obj, _cfruni(letter), cb, cbdata, cmp_f, cmp_data_f)
        else:
            if before_after == None:
                raise ValueError("need a valid after object to add an item before/after another item")
            if kind == ELM_INDEX_ITEM_INSERT_BEFORE:
                self.item = elm_index_item_insert_before(index.obj, before_after.item, _cfruni(letter), cb, cbdata)
            else:
                self.item = elm_index_item_insert_after(index.obj, before_after.item, _cfruni(letter), cb, cbdata)

        Py_INCREF(self)
        elm_object_item_del_cb_set(self.item, _index_item_del_cb)

    def selected_set(self, selected):
        """Set the selected state of an item.

        This sets the selected state of the given item.
        C{True} for selected, C{False} for not selected.

        If a new item is selected the previously selected will be unselected.
        Previously selected item can be get with function
        L{Index.selected_item_get()}.

        Selected items will be highlighted.

        @see: L{Index.selected_item_get()}

        @param selected: The selected state
        @type selected: bool

        """
        elm_index_item_selected_set(self.item, selected)

    def letter_get(self):
        """Get the letter (string) set on a given index widget item.

        @return: The letter string set on the item
        @rtype: string

        """
        return _ctouni(elm_index_item_letter_get(self.item))

cdef Elm_Object_Item *_elm_index_item_from_python(IndexItem item):
    if item is None:
        return NULL
    else:
        return item.item

cdef class Index(LayoutClass):

    """An index widget gives you an index for fast access to whichever
    group of other UI items one might have.

    It's a list of text items (usually letters, for alphabetically ordered
    access).

    Index widgets are by default hidden and just appear when the
    user clicks over it's reserved area in the canvas. In its
    default theme, it's an area one C{finger} wide on
    the right side of the index widget's container.

    When items on the index are selected, smart callbacks get
    called, so that its user can make other container objects to
    show a given area or child object depending on the index item
    selected. You'd probably be using an index together with L{List}s,
    L{Genlist}s or L{Gengrid}s.

    This widget emits the following signals, besides the ones sent from
    L{Layout}:
        - C{"changed"} - When the selected index item changes. C{event_info}
          is the selected item's data pointer.
        - C{"delay,changed"} - When the selected index item changes, but
          after a small idling period. C{event_info} is the selected
          item's data pointer.
        - C{"selected"} - When the user releases a mouse button and
          selects an item. C{event_info} is the selected item's data
          pointer.
        - C{"level,up"} - when the user moves a finger from the first
          level to the second level
        - C{"level,down"} - when the user moves a finger from the second
          level to the first level

    The C{"delay,changed"} event is so that it'll wait a small time
    before actually reporting those events and, moreover, just the
    last event happening on those time frames will actually be
    reported.

    """

    def __init__(self, c_evas.Object parent):
        Object.__init__(self, parent.evas)
        self._set_obj(elm_index_add(parent.obj))

    def autohide_disabled_set(self, disabled):
        """Enable or disable auto hiding feature for a given index widget.

        @see: L{autohide_disabled_get()}

        @param disabled: C{True} to disable auto hiding, C{False} to enable
        @type disabled: bool

        """
        elm_index_autohide_disabled_set(self.obj, disabled)

    def autohide_disabled_get(self):
        """Get whether auto hiding feature is enabled or not for a given index widget.

        @see: L{autohide_disabled_set()} for more details

        @return: C{True}, if auto hiding is disabled, C{False} otherwise
        @rtype: bool

        """
        return bool(elm_index_autohide_disabled_get(self.obj))

    property autohide_disabled:
        """Enable or disable auto hiding feature for a given index widget.

        @type: bool

        """
        def __get__(self):
            return self.autohide_disabled_get()
        def __set__(self, disabled):
            self.autohide_disabled_set(disabled)

    def item_level_set(self, level):
        """Set the items level for a given index widget.

        @see: L{item_level_get()}

        @param level: C{0} or C{1}, the currently implemented levels.
        @type level: int

        """
        elm_index_item_level_set(self.obj, level)

    def item_level_get(self):
        """Get the items level set for a given index widget.

        @see: L{item_level_set()} for more information

        @return: C{0} or C{1}, which are the levels the object might be at.
        @rtype: int

        """
        return elm_index_item_level_get(self.obj)

    property item_level:
        """The items level for a given index widget.

        C{0} or C{1}, the currently implemented levels.

        @type: int

        """
        def __get__(self):
            return self.item_level_get()
        def __set__(self, level):
            self.item_level_set(level)

    def selected_item_get(self, level):
        """Returns the last selected item, for a given index widget.

        @param level: C{0} or C{1}, the currently implemented levels.
        @type level: int
        @return: The last item B{selected} (or C{None}, on errors).
        @rtype: L{IndexItem}

        """
        cdef Elm_Object_Item *obj
        cdef void *d
        obj = elm_index_item_find(self.obj, <void*>level)
        if obj == NULL:
            return None
        d = elm_object_item_data_get(obj)
        if d == NULL:
            return None
        else:
            (o, callback, it, a, ka) = <object>d
            return it

    def item_append(self, letter, callback = None, *args, **kargs):
        """Append a new item on a given index widget.

        Despite the most common usage of the C{letter} argument is for
        single char strings, one could use arbitrary strings as index
        entries.

        C{item} will be the item returned back on C{"changed"},
        C{"delay,changed"} and C{"selected"} smart events.

        @param letter: Letter under which the item should be indexed
        @type letter: string
        @param callback: The function to call when the item is selected.
        @type callback: function
        @return: A handle to the item added or C{None}, on errors
        @rtype: L{IndexItem}

        """
        return IndexItem(ELM_INDEX_ITEM_INSERT_APPEND, self, letter,
                        None, callback, *args, **kargs)

    def item_prepend(self, letter, callback = None, *args, **kargs):
        """Prepend a new item on a given index widget.

        Despite the most common usage of the C{letter} argument is for
        single char strings, one could use arbitrary strings as index
        entries.

        C{item} will be the item returned back on C{"changed"},
        C{"delay,changed"} and C{"selected"} smart events.

        @param letter: Letter under which the item should be indexed
        @type letter: string
        @param callback: The function to call when the item is selected.
        @type callback: function
        @return: A handle to the item added or C{None}, on errors
        @rtype: L{IndexItem}

        """
        return IndexItem(ELM_INDEX_ITEM_INSERT_PREPEND, self, letter,
                        None, callback, *args, **kargs)

    def item_insert_after(self, IndexItem after, letter, callback = None, *args, **kargs):
        """Insert a new item into the index object after item C{after}.

        Despite the most common usage of the C{letter} argument is for
        single char strings, one could use arbitrary strings as index
        entries.

        C{item} will be the pointer returned back on C{"changed"},
        C{"delay,changed"} and C{"selected"} smart events.

        @note: If C{relative} is C{None} this function will behave as
            L{item_append()}.

        @param after: The index item to insert after.
        @type after: L{IndexItem}
        @param letter: Letter under which the item should be indexed
        @type letter: string
        @param func: The function to call when the item is clicked.
        @type func: function
        @return: A handle to the item added or C{None}, on errors
        @rtype: L{IndexItem}

        """
        return IndexItem(ELM_INDEX_ITEM_INSERT_AFTER, self, letter,
                        after, callback, *args, **kargs)

    def item_insert_before(self, IndexItem before, letter, callback = None, *args, **kargs):
        """Insert a new item into the index object before item C{before}.

        Despite the most common usage of the C{letter} argument is for
        single char strings, one could use arbitrary strings as index
        entries.

        C{item} will be the pointer returned back on C{"changed"},
        C{"delay,changed"} and C{"selected"} smart events.

        @note: If C{relative} is C{None} this function will behave as
            L{item_prepend()}.

        @param before: The index item to insert before.
        @type before: L{IndexItem}
        @param letter: Letter under which the item should be indexed
        @type letter: string
        @param func: The function to call when the item is clicked.
        @type func: function
        @return: A handle to the item added or C{None}, on errors
        @rtype: L{IndexItem}

        """
        return IndexItem(ELM_INDEX_ITEM_INSERT_BEFORE, self, letter,
                        before, callback, *args, **kargs)

    #def item_sorted_insert(self, letter, callback = None, *args, **kargs):
        """Insert a new item into the given index widget, using C{cmp_func}
        function to sort items (by item handles).

        Despite the most common usage of the C{letter} argument is for
        single char strings, one could use arbitrary strings as index
        entries.

        C{item} will be the pointer returned back on C{"changed"},
        C{"delay,changed"} and C{"selected"} smart events.

        @param letter: Letter under which the item should be indexed
        @type letter: string
        @param func: The function to call when the item is clicked.
        @type func: function
        @param cmp_func: The comparing function to be used to sort index
            items B{by index item handles}
        @type cmp_func: function
        @param cmp_data_func: A B{fallback} function to be called for the
            sorting of index items B{by item data}). It will be used
            when C{cmp_func} returns C{0} (equality), which means an index
            item with provided item data already exists. To decide which
            data item should be pointed to by the index item in question,
            C{cmp_data_func} will be used. If C{cmp_data_func} returns a
            non-negative value, the previous index item data will be
            replaced by the given C{item} pointer. If the previous data need
            to be freed, it should be done by the C{cmp_data_func} function,
            because all references to it will be lost. If this function is
            not provided (C{None} is given), index items will be B{duplicated},
            if C{cmp_func} returns C{0}.
        @type cmp_data_func: function
        @return: A handle to the item added or C{None}, on errors
        @rtype: L{IndexItem}

        """
        #return IndexItem(ELM_INDEX_ITEM_INSERT_SORTED, self, letter,
                        #None, callback, *args, **kargs)

    def item_find(self, data):
        """Find a given index widget's item, B{using item data}.

        @param data: The item data pointed to by the desired index item
        @return: The index item handle, if found, or C{None} otherwise
        @rtype: L{IndexItem}

        """
        cdef Elm_Object_Item *obj
        cdef void *d
        obj = elm_index_item_find(self.obj, <void*>data)
        if obj == NULL:
            return None
        d = elm_object_item_data_get(obj)
        if d == NULL:
            return None
        else:
            (o, callback, it, a, ka) = <object>d
            return it

    def item_clear(self):
        """Removes B{all} items from a given index widget.

        If deletion callbacks are set, via L{delete_cb_set()},
        that callback function will be called for each item.

        """
        elm_index_item_clear(self.obj)

    def level_go(self, level):
        """Go to a given items level on a index widget

        @param level: The index level (one of C{0} or C{1})
        @type level: int

        """
        elm_index_level_go(self.obj, level)

    def indicator_disabled_set(self, disabled):
        """Set the indicator as to be disabled.

        In Index widget, Indicator notes popup text, which shows a letter has been selecting.

        @see: L{indicator_disabled_get()}

        @param disabled: C{True} to disable it, C{False} to enable it
        @type disabled: bool

        """
        elm_index_indicator_disabled_set(self.obj, disabled)

    def indicator_disabled_get(self):
        """Get the value of indicator's disabled status.

        @see: L{indicator_disabled_set()}

        @return: True if the indicator is disabled.
        @rtype: bool

        """
        return bool(elm_index_indicator_disabled_get(self.obj))

    property indicator_disabled:
        """Whether the indicator is disabled or not.

        In Index widget, Indicator notes popup text, which shows a letter has been selecting.

        @type: bool

        """
        def __get__(self):
            return self.indicator_disabled_get()
        def __set__(self, disabled):
            self.indicator_disabled_set(disabled)

    def horizontal_set(self, horizontal):
        """Enable or disable horizontal mode on the index object

        On horizontal mode items are displayed on index from left to right,
        instead of from top to bottom. Also, the index will scroll horizontally.

        @see: L{horizontal_get()}

        @note: Vertical mode is set by default.

        @param horizontal: C{True} to enable horizontal or C{False} to
            disable it, i.e., to enable vertical mode. it's an area one
            C{"finger"} wide on the bottom side of the index widget's
            container.
        @type horizontal: bool

        """
        elm_index_horizontal_set(self.obj, horizontal)

    def horizontal_get(self):
        """Get a value whether horizontal mode is enabled or not.

        @see: L{horizontal_set()} for details.

        @return: C{True} means horizontal mode selection is enabled.
            C{False} indicates it's disabled.
        @rtype: bool

        """
        return bool(elm_index_horizontal_get(self.obj))

    property horizontal:
        """Enable or disable horizontal mode on the index object

        In horizontal mode items are displayed on index from left to right,
        instead of from top to bottom. Also, the index will scroll
        horizontally. It's an area one C{finger} wide on the bottom side of
        the index widget's container.

        @note: Vertical mode is set by default.

        @type: bool

        """
        def __get__(self):
            return self.horizontal_get()
        def __set__(self, horizontal):
            self.horizontal_set(horizontal)

    def callback_changed_add(self, func, *args, **kwargs):
        """When the selected index item changes. C{event_info} is the selected
        item's data."""
        self._callback_add_full("changed", _index_item_conv, func, *args, **kwargs)

    def callback_changed_del(self, func):
        self._callback_del_full("changed",  _index_item_conv, func)

    def callback_changed_delay_add(self, func, *args, **kwargs):
        """When the selected index item changes, but after a small idling
        period. C{event_info} is the selected item's data."""
        self._callback_add_full("changed,delay", _index_item_conv, func, *args, **kwargs)

    def callback_changed_delay_del(self, func):
        self._callback_del_full("changed,delay",  _index_item_conv, func)

    def callback_selected_add(self, func, *args, **kwargs):
        """When the user releases a mouse button and selects an item.
        C{event_info} is the selected item's data ."""
        self._callback_add_full("selected", _index_item_conv, func, *args, **kwargs)

    def callback_selected_del(self, func):
        self._callback_del_full("selected",  _index_item_conv, func)

    def callback_level_up_add(self, func, *args, **kwargs):
        """When the user moves a finger from the first level to the second
        level."""
        self._callback_add("level,up", func, *args, **kwargs)

    def callback_level_up_del(self, func):
        self._callback_del("level,up", func)

    def callback_level_down_add(self, func, *args, **kwargs):
        """When the user moves a finger from the second level to the first
        level."""
        self._callback_add("level,down", func, *args, **kwargs)

    def callback_level_down_del(self, func):
        self._callback_del("level,down", func)

_elm_widget_type_register("index", Index)
