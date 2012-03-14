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

cdef class Bubble(Object):
    def __init__(self, c_evas.Object parent):
        Object.__init__(self, parent.evas)
        self._set_obj(elm_bubble_add(parent.obj))

    def label_set(self, label):
        _METHOD_DEPRECATED(self, "text_set")
        self.text_set(label)

    def label_get(self):
        _METHOD_DEPRECATED(self, "text_get")
        return self.text_get()

    property label:
        def __get__(self):
            return self.label_get()

        def __set__(self, value):
            self.label_set(value)

    def info_set(self, info):
        _METHOD_DEPRECATED(self, "text_part_set", "use 'info' as part name")
        self.text_part_set("info", info)

    def content_set(self, c_evas.Object content):
        elm_object_part_content_set(self.obj, NULL, content.obj)

    def icon_set(self, c_evas.Object icon):
        elm_object_part_content_set(self.obj, "icon", icon.obj)

    def icon_get(self):
        cdef c_evas.Evas_Object *icon
        icon = elm_object_part_content_get(self.obj, "icon")
        return evas.c_evas._Object_from_instance(<long> icon)

    property icon:
        def __get__(self):
            return self.icon_get()

        def __set__(self, value):
            self.icon_set(value)

    def pos_set(self, pos):
        elm_bubble_pos_set(self.obj, pos)

_elm_widget_type_register("bubble", Bubble)
