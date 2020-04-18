/* Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From ppb_compositor.idl modified Tue Jun  3 12:44:44 2014. */

#ifndef PPAPI_C_PPB_COMPOSITOR_H_
#define PPAPI_C_PPB_COMPOSITOR_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_COMPOSITOR_INTERFACE_0_1 "PPB_Compositor;0.1" /* dev */
/**
 * @file
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * Defines the <code>PPB_Compositor</code> interface. Used for setting
 * <code>PPB_CompositorLayer</code> layers to the Chromium compositor for
 * compositing. This allows a plugin to combine different sources of visual
 * data efficiently, such as <code>PPB_ImageData</code> images and
 * OpenGL textures. See also <code>PPB_CompositorLayer</code> for more
 * information.
 * This interface is still in development (Dev API status) and may change,
 * so is only supported on Dev channel and Canary currently.
 *
 * <strong>Example usage from plugin code:</strong>
 *
 * <strong>Setup:</strong>
 * @code
 * PP_Resource compositor;
 * compositor = compositor_if->Create(instance);
 * instance_if->BindGraphics(instance, compositor);
 * @endcode
 *
 * <strong>Setup layer stack:</strong>
 * @code
 * PP_Resource color_layer = compositor_if->AddLayer(compositor);
 * PP_Resource texture_layer = compositor_if->AddLayer(compositor);
 * @endcode
 *
 * <strong> Present one frame:</strong>
 * layer_if->SetColor(color_layer, 255, 255, 0, 255, PP_MakeSize(400, 400));
 * PP_CompletionCallback release_callback = {
 *   TextureReleasedCallback, 0, PP_COMPLETIONCALLBACK_FLAG_NONE,
 * };
 * layer_if->SetTexture(texture_layer, graphics3d, texture_id,
 *                      PP_MakeSize(300, 300), release_callback);
 *
 * PP_CompletionCallback callback = {
 *   DidFinishCommitLayersCallback,
 *   (void*) texture_id,
 *   PP_COMPLETIONCALLBACK_FLAG_NONE,
 * };
 * compositor_if->CommitLayers(compositor, callback);
 * @endcode
 *
 * <strong>release callback</strong>
 * void ReleaseCallback(int32_t result, void* user_data) {
 *   if (result == PP_OK) {
 *     uint32_t texture_id = (uint32_t) user_data;
 *     // reuse the texture or delete it.
 *   }
 * }
 *
 * <strong>Shutdown:</strong>
 * @code
 * core->ReleaseResource(color_layer);
 * core->ReleaseResource(texture_layer);
 * core->ReleaseResource(compositor);
 * @endcode
 */
struct PPB_Compositor_0_1 { /* dev */
  /**
   * Determines if a resource is a compositor resource.
   *
   * @param[in] resource The <code>PP_Resource</code> to test.
   *
   * @return A <code>PP_Bool</code> with <code>PP_TRUE</code> if the given
   * resource is a compositor resource or <code>PP_FALSE</code> otherwise.
   */
  PP_Bool (*IsCompositor)(PP_Resource resource);
  /**
   * Creates a Compositor resource.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying one instance
   * of a module.
   *
   * @return A <code>PP_Resource</code> containing the compositor resource if
   * successful or 0 otherwise.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Creates a new <code>PPB_CompositorLayer</code> and adds it to the end
   * of the layer stack. A <code>PP_Resource</code> containing the layer is
   * returned. It is uninitialized, <code>SetColor()</code>,
   * <code>SetTexture</code> or <code>SetImage</code> should be used to
   * initialize it. The layer will appear above other pre-existing layers.
   * If <code>ResetLayers</code> is called or the <code>PPB_Compositor</code> is
   * released, the returned layer will be invalidated, and any further calls on
   * the layer will return <code>PP_ERROR_BADRESOURCE</code>.
   *
   * param[in] compositor A <code>PP_Resource</code> corresponding to
   * a compositor layer resource.
   *
   * @return A <code>PP_Resource</code> containing the compositor layer
   * resource if successful or 0 otherwise.
   */
  PP_Resource (*AddLayer)(PP_Resource compositor);
  /**
   * Commits layers added by <code>AddLayer()</code> to the chromium compositor.
   *
   * param[in] compositor A <code>PP_Resource</code> corresponding to
   * a compositor layer resource.
   * @param[in] cc A <code>PP_CompletionCallback</code> to be called when
   * layers have been represented on screen.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*CommitLayers)(PP_Resource compositor,
                          struct PP_CompletionCallback cc);
  /**
   * Resets layers added by <code>AddLayer()</code>.
   *
   * param[in] compositor A <code>PP_Resource</code> corresponding to
   * a compositor layer resource.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*ResetLayers)(PP_Resource compositor);
};
/**
 * @}
 */

#endif  /* PPAPI_C_PPB_COMPOSITOR_H_ */

