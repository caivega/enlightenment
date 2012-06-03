# Copyright (c) 2010 ProFUSION embedded systems
#
#This file is part of python-elementary.
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

cdef c_evas.Evas_Object *_tooltip_item_content_create(void *data, c_evas.Evas_Object *o, evas.c_evas.Evas_Object *t, void *it) with gil:
   cdef Object ret, obj, tooltip

   obj = <Object>c_evas.evas_object_data_get(o, "python-evas")
   tooltip = evas.c_evas._Object_from_instance(<long> t)
   (func, item, args, kargs) = <object>data
   ret = func(obj, item, *args, **kargs)
   if not ret:
       return NULL
   return ret.obj

cdef void _tooltip_item_data_del_cb(void *data, c_evas.Evas_Object *o, void *event_info) with gil:
   Py_DECREF(<object>data)

cdef class ObjectItem(object):

    """A generic item for the widgets.

    @group General: widget_get, part_content_set, content_set, part_content_get,
        content_get, part_content_unset, content_unset, part_text_set, text_set,
        part_text_get, text_get, text, access_info_set, data_get, signal_emit,
        tooltip_text_set, tooltip_window_mode_set, tooltip_window_mode_get,
        tooltip_content_cb_set, tooltip_unset, tooltip_style_set,
        tooltip_style_get, cursor_set, cursor_get, cursor_unset,
        cursor_style_set, cursor_style_get, cursor_engine_only_set,
        cursor_engine_only_get
    @group Styles: disabled_set, disabled_get, disabled

    """

    cdef void *base
    cdef Elm_Object_Item *obj

    def widget_get(self):
        """Get the widget object's handle which contains a given item

        @note: This returns the widget object itself that an item belongs to.
        @note: Every elm_object_item supports this API

        @return: The widget object
        @rtype: L{Object}

        """
        cdef c_evas.const_Evas_Object *obj = elm_object_item_widget_get(self.obj)
        return evas.c_evas._Object_from_instance(<long> obj)

    def part_content_set(self, char *part, Object obj):
        """Set a content of an object item

        This sets a new object to an item as a content object. If any object was
        already set as a content object in the same part, previous object will be
        deleted automatically.

        @note: Elementary object items may have many contents

        @param part: The content part name to set (None for the default content)
        @param content: The new content of the object item

        """
        elm_object_item_part_content_set(self.obj, part, obj.obj)

    def content_set(self, Object obj):
        elm_object_item_part_content_set(self.obj, NULL, obj.obj)

    def part_content_get(self, char *part):
        """Get a content of an object item

        @note: Elementary object items may have many contents

        @param part: The content part name to unset (None for the default content)
        @type part: string
        @return: content of the object item or None for any error
        @rtype: L{Object}

        """
        cdef c_evas.Evas_Object *obj = elm_object_item_part_content_get(self.obj, part)
        return evas.c_evas._Object_from_instance(<long> obj)

    def content_get(self):
        cdef c_evas.Evas_Object *obj = elm_object_item_content_get(self.obj)
        return evas.c_evas._Object_from_instance(<long> obj)

    def part_content_unset(self, char *part):
        """Unset a content of an object item

        @note: Elementary object items may have many contents

        @param part: The content part name to unset (None for the default content)
        @type part: string

        """
        cdef c_evas.Evas_Object *obj = elm_object_item_part_content_unset(self.obj, part)
        return evas.c_evas._Object_from_instance(<long> obj)

    def content_unset(self):
        cdef c_evas.Evas_Object *obj = elm_object_item_content_unset(self.obj)
        return evas.c_evas._Object_from_instance(<long> obj)


    def part_text_set(self, part, text):
        """Sets the text of a given part of this object.

        @see: L{text_set()} and L{part_text_get()}

        @param part: part name to set the text.
        @type part: string
        @param text: text to set.
        @type text: string
        """
        elm_object_item_part_text_set(self.obj, part, text)

    def text_set(self, text):
        """Sets the main text for this object.

        @see: L{text_get()} and L{part_text_set()}

        @param text: any text to set as the main textual part of this object.
        @type text: string
        """
        elm_object_item_text_set(self.obj, text)

    def part_text_get(self, part):
        """Gets the text of a given part of this object.

        @see: L{text_get()} and L{part_text_set()}

        @param part: part name to get the text.
        @type part: string
        @return: the text of a part or None if nothing was set.
        @rtype: string
        """
        cdef const_char_ptr l
        l = elm_object_item_part_text_get(self.obj, part)
        if l == NULL:
            return None
        return l

    def text_get(self):
        """Gets the main text for this object.

        @see: L{text_set()} and L{part_text_get()}

        @return: the main text or None if nothing was set.
        @rtype: string
        """
        cdef const_char_ptr l
        l = elm_object_item_text_get(self.obj)
        if l == NULL:
            return None
        return l

    property text:
        """The main text for this object.

        @type: string

        """
        def __get__(self):
            return self.text_get()

        def __set__(self, value):
            self.text_set(value)

    def access_info_set(self, txt):
        """Set the text to read out when in accessibility mode

        @param txt: The text that describes the widget to people with poor or no vision
        @type txt: string

        """
        elm_object_item_access_info_set(self.obj, txt)

    def data_get(self):
        """Returns the callback data given at creation time.

        @rtype: tuple of (args, kargs), args is tuple, kargs is dict.
        """
        cdef void* data
        data = elm_object_item_data_get(self.obj)
        if data == NULL:
            return None
        else:
            (obj, callback, it, a, ka) = <object>data

            return (a, ka)

    #def data_set(self, data):
        #elm_object_item_data_set(self.obj, <void*>data)

    property data:
        def __get__(self):
            return self.data_get()
        #def __set__(self, data):
            #self.data_set(data)

    def signal_emit(self, emission, source):
        """Send a signal to the edje object of the widget item.

        This function sends a signal to the edje object of the obj item. An
        edje program can respond to a signal by specifying matching
        'signal' and 'source' fields.

        @param emission: The signal's name.
        @type emission: string
        @param source: The signal's source.
        @type source: string

        """
        elm_object_item_signal_emit(self.obj, emission, source)

    def disabled_set(self, disabled):
        """Set the disabled state of an widget item.

        Elementary object item can be B{disabled}, in which state they won't
        receive input and, in general, will be themed differently from
        their normal state, usually greyed out. Useful for contexts
        where you don't want your users to interact with some of the
        parts of you interface.

        This sets the state for the widget item, either disabling it or
        enabling it back.

        @param disabled: The state to put in in: C{True} for
            disabled, C{False} for enabled
        @type disabled: bool

        """
        elm_object_item_disabled_set(self.obj, disabled)

    def disabled_get(self):
        """Get the disabled state of an widget item.

        This gets the state of the widget, which might be enabled or disabled.

        @return: C{True}, if the widget item is disabled, C{False}
            if it's enabled (or on errors)
        @rtype: bool

        """
        return bool(elm_object_item_disabled_get(self.obj))

    property disabled:
        """The disabled state of an widget item.

        Elementary object item can be B{disabled}, in which state they won't
        receive input and, in general, will be themed differently from
        their normal state, usually greyed out. Useful for contexts
        where you don't want your users to interact with some of the
        parts of you interface.

        @type: bool

        """
        def __get__(self):
            return self.disabled_get()
        def __set__(self, disabled):
            self.disabled_set(disabled)

    #def delete_cb_set(self, del_cb):
        #elm_object_item_del_cb_set(self.obj, del_cb)

    def delete(self):
        if self.obj == NULL:
            raise ValueError("Object already deleted")
        elm_object_item_del(self.obj)

    def tooltip_text_set(self, char *text):
        """Set the text to be shown in the tooltip object

        Setup the text as tooltip object. The object can have only one
        tooltip, so any previous tooltip data is removed.
        Internally, this method calls L{tooltip_content_cb_set}
        """
        elm_object_item_tooltip_text_set(self.obj, text)

    def tooltip_window_mode_set(self, disable):
        return bool(elm_object_item_tooltip_window_mode_set(self.obj, disable))

    def tooltip_window_mode_get(self):
        return bool(elm_object_item_tooltip_window_mode_get(self.obj))

    def tooltip_content_cb_set(self, func, *args, **kargs):
        """Set the content to be shown in the tooltip object

        Setup the tooltip to object. The object can have only one
        tooltip, so any previews tooltip data is removed. C{func(owner,
        tooltip, args, kargs)} will be called every time that need
        show the tooltip and it should return a valid
        Evas_Object. This object is then managed fully by tooltip
        system and is deleted when the tooltip is gone.

        @param func: Function to be create tooltip content, called when
            need show tooltip.
        """
        if not callable(func):
            raise TypeError("func must be callable")

        cdef void *cbdata

        data = (func, args, kargs)
        Py_INCREF(data)
        cbdata = <void *>data
        elm_object_item_tooltip_content_cb_set(self.obj, _tooltip_item_content_create,
                                          cbdata, _tooltip_item_data_del_cb)

    def tooltip_unset(self):
        """Unset tooltip from object

        Remove tooltip from object. If used the L{tooltip_text_set} the internal
        copy of label will be removed correctly. If used
        L{tooltip_content_cb_set}, the data will be unreferred but no freed.
        """
        elm_object_item_tooltip_unset(self.obj)

    def tooltip_style_set(self, style=None):
        """Sets a different style for this object tooltip.

        @note: before you set a style you should define a tooltip with
        L{tooltip_content_cb_set()} or L{tooltip_text_set()}
        """
        if style:
            elm_object_item_tooltip_style_set(self.obj, style)
        else:
            elm_object_item_tooltip_style_set(self.obj, NULL)

    def tooltip_style_get(self):
        """Get the style for this object tooltip."""
        cdef const_char_ptr style
        style = elm_object_item_tooltip_style_get(self.obj)
        if style == NULL:
            return None
        return style

    def cursor_set(self, char *cursor):
        """Set the cursor to be shown when mouse is over the object

        Set the cursor that will be displayed when mouse is over the
        object. The object can have only one cursor set to it, so if
        this function is called twice for an object, the previous set
        will be unset.
        """
        elm_object_item_cursor_set(self.obj, cursor)

    def cursor_get(self):
        return elm_object_item_cursor_get(self.obj)

    def cursor_unset(self):
        """Unset cursor for object

        Unset cursor for object, and set the cursor to default if the mouse
        was over this object.
        """
        elm_object_item_cursor_unset(self.obj)

    def cursor_style_set(self, style=None):
        """Sets a different style for this object cursor.

        @note: before you set a style you should define a cursor with
        L{cursor_set()}
        """
        if style:
            elm_object_item_cursor_style_set(self.obj, style)
        else:
            elm_object_item_cursor_style_set(self.obj, NULL)

    def cursor_style_get(self):
        """Get the style for this object cursor."""
        cdef const_char_ptr style
        style = elm_object_item_cursor_style_get(self.obj)
        if style == NULL:
            return None
        return style

    def cursor_engine_only_set(self, engine_only):
        """Sets cursor engine only usage for this object.

        @note: before you set engine only usage you should define a cursor with
        L{cursor_set()}
        """
        elm_object_item_cursor_engine_only_set(self.obj, bool(engine_only))

    def cursor_engine_only_get(self):
        """Get the engine only usage for this object."""
        return elm_object_item_cursor_engine_only_get(self.obj)

_elm_widget_type_register("object_item", ObjectItem)
