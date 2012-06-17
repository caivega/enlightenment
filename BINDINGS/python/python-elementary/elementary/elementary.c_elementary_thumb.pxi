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

cdef public class Thumb(Object) [object PyElementaryThumb, type PyElementaryThumb_Type]:

    """A thumbnail object is used for displaying the thumbnail of an image
    or video. You must have compiled Elementary with C{Ethumb_Client}
    support. Also, Ethumb's DBus service must be present and
    auto-activated in order to have thumbnails generated. You must also
    have a B{session} bus, not a B{system} one.

    Once the thumbnail object becomes visible, it will check if there
    is a previously generated thumbnail image for the file set on
    it. If not, it will start generating this thumbnail.

    Different configuration settings will cause different thumbnails to
    be generated even on the same file.

    Generated thumbnails are stored under C{$HOME/.thumbnails/}. Check
    Ethumb's documentation to change this path, and to see other
    configuration options.

    This widget emits the following signals:
        - C{"clicked"} - This is called when a user has clicked the
                         thumbnail object without dragging it around.
        - C{"clicked,double"} - This is called when a user has double-clicked
                                the thumbnail object.
        - C{"press"} - This is called when a user has pressed down over the
                       thumbnail object.
        - C{"generate,start"} - The thumbnail generation has started.
        - C{"generate,stop"} - The generation process has stopped.
        - C{"generate,error"} - The thumbnail generation failed.
        - C{"load,error"} - The thumbnail image loading failed.

    Available styles:
        - C{"default"}
        - C{"noframe"}

    """

    def __init__(self, evasObject parent):
        elm_need_ethumb()
        Object.__init__(self, parent.evas)
        self._set_obj(elm_thumb_add(parent.obj))

    def reload(self):
        """Reload thumbnail if it was generated before.

        This is useful if the ethumb client configuration changed, like its
        size, aspect or any other property one set in the handle returned
        by L{ethumb_client_get()}.

        If the options didn't change, the thumbnail won't be generated
        again, but the old one will still be used.

        @see: L{file_set()}

        """
        elm_thumb_reload(self.obj)

    property file:
        """The file that will be used as thumbnail B{source}.

        The file can be an image or a video (in that case, acceptable
        extensions are: avi, mp4, ogv, mov, mpg and wmv). To start the
        video animation, use the function L{animate()}.

        @see: L{reload()}
        @see: L{animate()}

        @type: (string file, optional string eet_key)

        """
        def __set__(self, value):
            if isinstance(value, tuple) or isinstance(value, list):
                file, key = value
            else:
                file = value
                key = ""
            elm_thumb_file_set(self.obj, _cfruni(file), _cfruni(key))
        def __get__(self):
            cdef const_char_ptr file, key
            elm_thumb_file_get(self.obj, &file, &key)
            return(_ctouni(file), _ctouni(key))

    property path:
        """Get the path and key to the image or video thumbnail generated by
        ethumb.

        One just needs to make sure that the thumbnail was generated before
        getting its path; otherwise, the path will be None. One way to do
        that is by asking for the path when/after the "generate,stop" smart
        callback is called.

        @see: L{file}

        @type: (string path, string eet_key)

        """
        def __get__(self):
            cdef const_char_ptr path, key
            elm_thumb_path_get(self.obj, &path, &key)
            return(_ctouni(path), _ctouni(key))

    property animate:
        """Set the animation state for the thumb object. If its content is
        an animated video, you may start/stop the animation or tell it to
        play continuously and looping.

        @see: L{file}

        @type: Elm_Thumb_Animation_Setting

        """
        def __set__(self, s):
            elm_thumb_animate_set(self.obj, s)
        def __get__(self):
            return elm_thumb_animate_get(self.obj)

    def ethumb_client_get(self):
        """Get the ethumb_client handle so custom configuration can be made.

        This must be called before the objects are created to be sure no object is
        visible and no generation started.

        Example of usage::
            #include <Elementary.h>
            #ifndef ELM_LIB_QUICKLAUNCH
            EAPI_MAIN int
            elm_main(int argc, char **argv)
            {
               Ethumb_Client *client;

               elm_need_ethumb();

               // ... your code

               client = elm_thumb_ethumb_client_get();
               if (!client)
                 {
                    ERR("could not get ethumb_client");
                    return 1;
                 }
               ethumb_client_size_set(client, 100, 100);
               ethumb_client_crop_align_set(client, 0.5, 0.5);
               // ... your code

               // Create elm_thumb objects here

               elm_run();
               elm_shutdown();
               return 0;
            }
            #endif
            ELM_MAIN()

        @note: There's only one client handle for Ethumb, so once a
            configuration change is done to it, any other request for
            thumbnails (for any thumbnail object) will use that
            configuration. Thus, this configuration is global.

        @return: Ethumb_Client instance or None.
        @rtype: Ethumb_Client

        """
        return
        #return elm_thumb_ethumb_client_get(void)

    def ethumb_client_connected_get(self):
        """Get the ethumb_client connection state.

        @return: True if the client is connected to the server or False
            otherwise.
        @rtype: bool

        """
        return bool(elm_thumb_ethumb_client_connected_get())

    property editable:
        """Make the thumbnail 'editable'.

        This means the thumbnail is a valid drag target for drag and drop,
        and can be cut or pasted too.

        @type: bool

        """
        def __set__(self, edit):
            elm_thumb_editable_set(self.obj, edit)
        def __get__(self):
            return bool(elm_thumb_editable_get(self.obj))

    def callback_clicked_add(self, func, *args, **kwargs):
        """This is called when a user has clicked the thumbnail object
        without dragging it around."""
        self._callback_add("clicked", func, *args, **kwargs)

    def callback_clicked_del(self, func):
        self._callback_del("clicked", func)

    def callback_clicked_double_add(self, func, *args, **kwargs):
        """This is called when a user has double-clicked the thumbnail
        object."""
        self._callback_add("clicked,double", func, *args, **kwargs)

    def callback_clicked_double_del(self, func):
        self._callback_del("clicked,double", func)

    def callback_press_add(self, func, *args, **kwargs):
        """This is called when a user has pressed down over the thumbnail
        object."""
        self._callback_add("press", func, *args, **kwargs)

    def callback_press_del(self, func):
        self._callback_del("press", func)

    def callback_generate_start_add(self, func, *args, **kwargs):
        """The thumbnail generation has started."""
        self._callback_add("generate,start", func, *args, **kwargs)

    def callback_generate_start_del(self, func):
        self._callback_del("generate,start", func)

    def callback_generate_stop_add(self, func, *args, **kwargs):
        """The generation process has stopped."""
        self._callback_add("generate,stop", func, *args, **kwargs)

    def callback_generate_stop_del(self, func):
        self._callback_del("generate,stop", func)

    def callback_generate_error_add(self, func, *args, **kwargs):
        """The thumbnail generation failed."""
        self._callback_add("generate,error", func, *args, **kwargs)

    def callback_generate_error_del(self, func):
        self._callback_del("generate,error", func)

    def callback_load_error_add(self, func, *args, **kwargs):
        """The thumbnail image loading failed."""
        self._callback_add("load,error", func, *args, **kwargs)

    def callback_load_error_del(self, func):
        self._callback_del("load,error", func)

_elm_widget_type_register("thumb", Thumb)

cdef extern from "Elementary.h": # hack to force type to be known
    cdef PyTypeObject PyElementaryThumb_Type # hack to install metaclass
_install_metaclass(&PyElementaryThumb_Type, ElementaryObjectMeta)
