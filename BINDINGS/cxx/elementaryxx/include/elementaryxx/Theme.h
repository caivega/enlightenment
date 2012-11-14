#ifndef ELMXX_THEME_H
#define ELMXX_THEME_H

/* STL */
#include <string>

/* EFL */
#include <Elementary.h>

/* ELFxx */
#include "Object.h"

namespace Elmxx {

/*!
 * smart callbacks called:
 * "changed" - the user toggled the state
 */
class Theme
{
public:
  Theme ();
  ~Theme ();
  Theme (const Theme &th); // specific copy constructor

  /**
   * Prepends a theme overlay to the list of overlays
   *
   * @param item The Edje file path to be used
   *
   * Use this if your application needs to provide some custom overlay theme
   * (An Edje file that replaces some default styles of widgets) where adding
   * new styles, or changing system theme configuration is not possible. Do
   * NOT use this instead of a proper system theme configuration. Use proper
   * configuration files, profiles, environment variables etc. to set a theme
   * so that the theme can be altered by simple configuration by a user. Using
   * this call to achieve that effect is abusing the API and will create lots
   * of trouble.
   *
   * @see addExtension()
   *
   * @ingroup Theme
   */
  void addOverlay(const std::string& item);

  /**
   * Delete a theme overlay from the list of overlays
   *
   * @param item The name of the theme overlay
   *
   * @see addOverlay()
   *
   * @ingroup Theme
   */
  void delOverlay(const std::string& item);
  
  /**
   * Appends a theme extension to the list of extensions.
   *
   * @param item The Edje file path to be used
   *
   * This is intended when an application needs more styles of widgets or new
   * widget themes that the default does not provide (or may not provide). The
   * application has "extended" usage by coming up with new custom style names
   * for widgets for specific uses, but as these are not "standard", they are
   * not guaranteed to be provided by a default theme. This means the
   * application is required to provide these extra elements itself in specific
   * Edje files. This call adds one of those Edje files to the theme search
   * path to be search after the default theme. The use of this call is
   * encouraged when default styles do not meet the needs of the application.
   * Use this call instead of elm_theme_overlay_add() for almost all cases.
   *
   * @see setStyle()
   *
   * @ingroup Theme
   */
  void addExtension (const std::string& item);

  /**
   * Deletes a theme extension from the list of extensions.
   *
   * @param item The name of the theme extension
   *
   * @see addExtension()
   *
   * @ingroup Theme
   */
  void delExtension (const std::string& item);
  
private:
  Elm_Theme *mTheme;
};



#if 0

/**
 * Tell the source theme to reference the ref theme
 *
 * @param th The theme that will do the referencing
 * @param thref The theme that is the reference source
 *
 * This clears @p th to be empty and then sets it to refer to @p thref
 * so @p th acts as an override to @p thref, but where its overrides
 * don't apply, it will fall through to @p thref for configuration.
 *
 * @ingroup Theme
 */
EAPI void             elm_theme_ref_set(Elm_Theme *th, Elm_Theme *thref);

/**
 * Return the theme referred to
 *
 * @param th The theme to get the reference from
 * @return The referenced theme handle
 *
 * This gets the theme set as the reference theme by elm_theme_ref_set().
 * If no theme is set as a reference, NULL is returned.
 *
 * @ingroup Theme
 */
EAPI Elm_Theme       *elm_theme_ref_get(Elm_Theme *th);

/**
 * Return the default theme
 *
 * @return The default theme handle
 *
 * This returns the internal default theme setup handle that all widgets
 * use implicitly unless a specific theme is set. This is also often use
 * as a shorthand of NULL.
 *
 * @ingroup Theme
 */
EAPI Elm_Theme       *elm_theme_default_get(void);

/**
 * Get the list of registered overlays for the given theme
 *
 * @param th The theme from which to get the overlays
 * @return List of theme overlays. Do not free it.
 *
 * @see elm_theme_overlay_add()
 *
 * @ingroup Theme
 */
EAPI const Eina_List *elm_theme_overlay_list_get(const Elm_Theme *th);


/**
 * Get the list of registered extensions for the given theme
 *
 * @param th The theme from which to get the extensions
 * @return List of theme extensions. Do not free it.
 *
 * @see elm_theme_extension_add()
 *
 * @ingroup Theme
 */
EAPI const Eina_List *elm_theme_extension_list_get(const Elm_Theme *th);

/**
 * Set the theme search order for the given theme
 *
 * @param th The theme to set the search order, or if NULL, the default theme
 * @param theme Theme search string
 *
 * This sets the search string for the theme in path-notation from first
 * theme to search, to last, delimited by the : character. Example:
 *
 * "shiny:/path/to/file.edj:default"
 *
 * See the ELM_THEME environment variable for more information.
 *
 * @see elm_theme_get()
 * @see elm_theme_list_get()
 *
 * @ingroup Theme
 */
EAPI void             elm_theme_set(Elm_Theme *th, const char *theme);

/**
 * Return the theme search order
 *
 * @param th The theme to get the search order, or if NULL, the default theme
 * @return The internal search order path
 *
 * This function returns a colon separated string of theme elements as
 * returned by elm_theme_list_get().
 *
 * @see elm_theme_set()
 * @see elm_theme_list_get()
 *
 * @ingroup Theme
 */
EAPI const char      *elm_theme_get(Elm_Theme *th);

/**
 * Return a list of theme elements to be used in a theme.
 *
 * @param th Theme to get the list of theme elements from.
 * @return The internal list of theme elements
 *
 * This returns the internal list of theme elements (will only be valid as
 * long as the theme is not modified by elm_theme_set() or theme is not
 * freed by elm_theme_free(). This is a list of strings which must not be
 * altered as they are also internal. If @p th is NULL, then the default
 * theme element list is returned.
 *
 * A theme element can consist of a full or relative path to a .edj file,
 * or a name, without extension, for a theme to be searched in the known
 * theme paths for Elementary.
 *
 * @see elm_theme_set()
 * @see elm_theme_get()
 *
 * @ingroup Theme
 */
EAPI const Eina_List *elm_theme_list_get(const Elm_Theme *th);

/**
 * Return the full path for a theme element
 *
 * @param f The theme element name
 * @param in_search_path Pointer to a boolean to indicate if item is in the search path or not
 * @return The full path to the file found.
 *
 * This returns a string you should free with free() on success, NULL on
 * failure. This will search for the given theme element, and if it is a
 * full or relative path element or a simple search-able name. The returned
 * path is the full path to the file, if searched, and the file exists, or it
 * is simply the full path given in the element or a resolved path if
 * relative to home. The @p in_search_path boolean pointed to is set to
 * EINA_TRUE if the file was a search-able file and is in the search path,
 * and EINA_FALSE otherwise.
 *
 * @ingroup Theme
 */
EAPI char            *elm_theme_list_item_path_get(const char *f, Eina_Bool *in_search_path);

/**
 * Flush the current theme.
 *
 * @param th Theme to flush
 *
 * This flushes caches that let elementary know where to find theme elements
 * in the given theme. If @p th is NULL, then the default theme is flushed.
 * Call this function if source theme data has changed in such a way as to
 * make any caches Elementary kept invalid.
 *
 * @ingroup Theme
 */
EAPI void             elm_theme_flush(Elm_Theme *th);

/**
 * This flushes all themes (default and specific ones).
 *
 * This will flush all themes in the current application context, by calling
 * elm_theme_flush() on each of them.
 *
 * @ingroup Theme
 */
EAPI void             elm_theme_full_flush(void);

/**
 * Return a list of theme elements in the theme search path
 *
 * @return A list of strings that are the theme element names.
 *
 * This lists all available theme files in the standard Elementary search path
 * for theme elements, and returns them in alphabetical order as theme
 * element names in a list of strings. Free this with
 * elm_theme_name_available_list_free() when you are done with the list.
 *
 * @ingroup Theme
 */
EAPI Eina_List       *elm_theme_name_available_list_new(void);

/**
 * Free the list returned by elm_theme_name_available_list_new()
 *
 * This frees the list of themes returned by
 * elm_theme_name_available_list_new(). Once freed the list should no longer
 * be used. a new list mys be created.
 *
 * @ingroup Theme
 */
EAPI void             elm_theme_name_available_list_free(Eina_List *list);

/**
 * Set a specific theme to be used for this object and its children
 *
 * @param obj The object to set the theme on
 * @param th The theme to set
 *
 * This sets a specific theme that will be used for the given object and any
 * child objects it has. If @p th is NULL then the theme to be used is
 * cleared and the object will inherit its theme from its parent (which
 * ultimately will use the default theme if no specific themes are set).
 *
 * Use special themes with great care as this will annoy users and make
 * configuration difficult. Avoid any custom themes at all if it can be
 * helped.
 *
 * @ingroup Theme
 */
EAPI void             elm_object_theme_set(Evas_Object *obj, Elm_Theme *th);

/**
 * Get the specific theme to be used
 *
 * @param obj The object to get the specific theme from
 * @return The specific theme set.
 *
 * This will return a specific theme set, or NULL if no specific theme is
 * set on that object. It will not return inherited themes from parents, only
 * the specific theme set for that specific object. See elm_object_theme_set()
 * for more information.
 *
 * @ingroup Theme
 */
EAPI Elm_Theme       *elm_object_theme_get(const Evas_Object *obj);

/**
 * Get a data item from a theme
 *
 * @param th The theme, or NULL for default theme
 * @param key The data key to search with
 * @return The data value, or NULL on failure
 *
 * This function is used to return data items from edc in @p th, an overlay, or an extension.
 * It works the same way as edje_file_data_get() except that the return is stringshared.
 *
 * @ingroup Theme
 */
EAPI const char      *elm_theme_data_get(Elm_Theme *th, const char *key);

#endif

} // end namespace Elmxx

#endif // ELMXX_THEME_H
