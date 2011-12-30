/**
 * @defgroup Mapbuf Mapbuf
 * @ingroup Elementary
 *
 * @image html img/widget/mapbuf/preview-00.png
 * @image latex img/widget/mapbuf/preview-00.eps width=\textwidth
 *
 * This holds one content object and uses an Evas Map of transformation
 * points to be later used with this content. So the content will be
 * moved, resized, etc as a single image. So it will improve performance
 * when you have a complex interafce, with a lot of elements, and will
 * need to resize or move it frequently (the content object and its
 * children).
 *
 * Default contents parts of the mapbuf widget that you can use for are:
 * @li "default" - A content of the mapbuf
 *
 * To enable map, elm_mapbuf_enabled_set() should be used.
 *
 * See how to use this widget in this example:
 * @ref mapbuf_example
 */

/**
 * @addtogroup Mapbuf
 * @{
 */

/**
 * Add a new mapbuf widget to the given parent Elementary
 * (container) object.
 *
 * @param parent The parent object.
 * @return A new mapbuf widget handle or @c NULL, on errors.
 *
 * This function inserts a new mapbuf widget on the canvas.
 *
 * @ingroup Mapbuf
 */
EAPI Evas_Object *
                                  elm_mapbuf_add(Evas_Object *parent)
EINA_ARG_NONNULL(1);

/**
 * Set the content of the mapbuf.
 *
 * @param obj The mapbuf object.
 * @param content The content that will be filled in this mapbuf object.
 *
 * Once the content object is set, a previously set one will be deleted.
 * If you want to keep that old content object, use the
 * elm_mapbuf_content_unset() function.
 *
 * To enable map, elm_mapbuf_enabled_set() should be used.
 *
 * @deprecated use elm_object_content_set() instead
 *
 * @ingroup Mapbuf
 */
EINA_DEPRECATED EAPI void         elm_mapbuf_content_set(Evas_Object *obj, Evas_Object *content) EINA_ARG_NONNULL(1);

/**
 * Get the content of the mapbuf.
 *
 * @param obj The mapbuf object.
 * @return The content that is being used.
 *
 * Return the content object which is set for this widget.
 *
 * @see elm_mapbuf_content_set() for details.
 *
 * @deprecated use elm_object_content_get() instead
 *
 * @ingroup Mapbuf
 */
EINA_DEPRECATED EAPI Evas_Object *elm_mapbuf_content_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Unset the content of the mapbuf.
 *
 * @param obj The mapbuf object.
 * @return The content that was being used.
 *
 * Unparent and return the content object which was set for this widget.
 *
 * @see elm_mapbuf_content_set() for details.
 *
 * @deprecated use elm_object_content_unset() instead
 *
 * @ingroup Mapbuf
 */
EINA_DEPRECATED EAPI Evas_Object *elm_mapbuf_content_unset(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Enable or disable the map.
 *
 * @param obj The mapbuf object.
 * @param enabled @c EINA_TRUE to enable map or @c EINA_FALSE to disable it.
 *
 * This enables the map that is set or disables it. On enable, the object
 * geometry will be saved, and the new geometry will change (position and
 * size) to reflect the map geometry set.
 *
 * Also, when enabled, alpha and smooth states will be used, so if the
 * content isn't solid, alpha should be enabled, for example, otherwise
 * a black retangle will fill the content.
 *
 * When disabled, the stored map will be freed and geometry prior to
 * enabling the map will be restored.
 *
 * It's disabled by default.
 *
 * @see elm_mapbuf_alpha_set()
 * @see elm_mapbuf_smooth_set()
 *
 * @ingroup Mapbuf
 */
EAPI void                         elm_mapbuf_enabled_set(Evas_Object *obj, Eina_Bool enabled) EINA_ARG_NONNULL(1);

/**
 * Get a value whether map is enabled or not.
 *
 * @param obj The mapbuf object.
 * @return @c EINA_TRUE means map is enabled. @c EINA_FALSE indicates
 * it's disabled. If @p obj is @c NULL, @c EINA_FALSE is returned.
 *
 * @see elm_mapbuf_enabled_set() for details.
 *
 * @ingroup Mapbuf
 */
EAPI Eina_Bool                    elm_mapbuf_enabled_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Enable or disable smooth map rendering.
 *
 * @param obj The mapbuf object.
 * @param smooth @c EINA_TRUE to enable smooth map rendering or @c EINA_FALSE
 * to disable it.
 *
 * This sets smoothing for map rendering. If the object is a type that has
 * its own smoothing settings, then both the smooth settings for this object
 * and the map must be turned off.
 *
 * By default smooth maps are enabled.
 *
 * @ingroup Mapbuf
 */
EAPI void                         elm_mapbuf_smooth_set(Evas_Object *obj, Eina_Bool smooth) EINA_ARG_NONNULL(1);

/**
 * Get a value whether smooth map rendering is enabled or not.
 *
 * @param obj The mapbuf object.
 * @return @c EINA_TRUE means smooth map rendering is enabled. @c EINA_FALSE
 * indicates it's disabled. If @p obj is @c NULL, @c EINA_FALSE is returned.
 *
 * @see elm_mapbuf_smooth_set() for details.
 *
 * @ingroup Mapbuf
 */
EAPI Eina_Bool                    elm_mapbuf_smooth_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Set or unset alpha flag for map rendering.
 *
 * @param obj The mapbuf object.
 * @param alpha @c EINA_TRUE to enable alpha blending or @c EINA_FALSE
 * to disable it.
 *
 * This sets alpha flag for map rendering. If the object is a type that has
 * its own alpha settings, then this will take precedence. Only image objects
 * have this currently. It stops alpha blending of the map area, and is
 * useful if you know the object and/or all sub-objects is 100% solid.
 *
 * Alpha is enabled by default.
 *
 * @ingroup Mapbuf
 */
EAPI void                         elm_mapbuf_alpha_set(Evas_Object *obj, Eina_Bool alpha) EINA_ARG_NONNULL(1);

/**
 * Get a value whether alpha blending is enabled or not.
 *
 * @param obj The mapbuf object.
 * @return @c EINA_TRUE means alpha blending is enabled. @c EINA_FALSE
 * indicates it's disabled. If @p obj is @c NULL, @c EINA_FALSE is returned.
 *
 * @see elm_mapbuf_alpha_set() for details.
 *
 * @ingroup Mapbuf
 */
EAPI Eina_Bool                    elm_mapbuf_alpha_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * @}
 */
