/* Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From ppb_compositor_layer.idl modified Thu Aug 14 18:06:33 2014. */

#ifndef PPAPI_C_PPB_COMPOSITOR_LAYER_H_
#define PPAPI_C_PPB_COMPOSITOR_LAYER_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_COMPOSITORLAYER_INTERFACE_0_1 "PPB_CompositorLayer;0.1" /* dev */
#define PPB_COMPOSITORLAYER_INTERFACE_0_2 "PPB_CompositorLayer;0.2" /* dev */
/**
 * @file
 */


/**
 * @addtogroup Enums
 * @{
 */
/**
 * This enumeration contains blend modes used for computing the result pixels
 * based on the source RGBA values in layers with the RGBA values that are
 * already in the destination framebuffer.
 * alpha_src, color_src: source alpha and color.
 * alpha_dst, color_dst: destination alpha and color (before compositing).
 * Below descriptions of the blend modes assume the colors are pre-multiplied.
 * This interface is still in development (Dev API status) and may change,
 * so is only supported on Dev channel and Canary currently.
 */
typedef enum {
  /**
   * No blending, copy source to the destination directly.
   */
  PP_BLENDMODE_NONE,
  /**
   * Source is placed over the destination.
   * Resulting alpha = alpha_src + alpha_dst - alpha_src * alpha_dst
   * Resulting color = color_src + color_dst * (1 - alpha_src)
   */
  PP_BLENDMODE_SRC_OVER,
  /**
   * The last blend mode.
   */
  PP_BLENDMODE_LAST = PP_BLENDMODE_SRC_OVER
} PP_BlendMode;
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * Defines the <code>PPB_CompositorLayer</code> interface. It is used by
 * <code>PPB_Compositor</code>.
 */
struct PPB_CompositorLayer_0_2 { /* dev */
  /**
   * Determines if a resource is a compositor layer resource.
   *
   * @param[in] resource The <code>PP_Resource</code> to test.
   *
   * @return A <code>PP_Bool</code> with <code>PP_TRUE</code> if the given
   * resource is a compositor layer resource or <code>PP_FALSE</code>
   * otherwise.
   */
  PP_Bool (*IsCompositorLayer)(PP_Resource resource);
  /**
   * Sets the color of a solid color layer. If the layer is uninitialized,
   * it will initialize the layer first, and then set its color.
   * If the layer has been initialized to another kind of layer, the layer will
   * not be changed, and <code>PP_ERROR_BADARGUMENT</code> will be returned.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] red A <code>float</code> for the red color component. It will be
   * clamped to [0, 1].
   * param[in] green A <code>float</code> for the green color component. It will
   * be clamped to [0, 1].
   * param[in] blue A <code>float</code> for the blue color component. It will
   * be clamped to [0, 1].
   * param[in] alpha A <code>float</code> for the alpha color component. It will
   * be clamped to [0, 1].
   * param[in] size A <code>PP_Size</code> for the size of the layer before
   * transform.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetColor)(PP_Resource layer,
                      float red,
                      float green,
                      float blue,
                      float alpha,
                      const struct PP_Size* size);
  /**
   * Sets the texture of a texture layer. If the layer is uninitialized,
   * it will initialize the layer first, and then set its texture.
   * The source rect will be set to ((0, 0), (1, 1)). If the layer has been
   * initialized to another kind of layer, the layer will not be changed,
   * and <code>PP_ERROR_BADARGUMENT</code> will be returned.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] context A <code>PP_Resource</code> corresponding to a graphics
   * 3d resource which owns the GL texture.
   * param[in] target GL texture target (GL_TEXTURE_2D, etc).
   * param[in] texture A GL texture object id.
   * param[in] size A <code>PP_Size</code> for the size of the layer before
   * transform.
   * param[in] cc A <code>PP_CompletionCallback</code> to be called when
   * the texture is released by Chromium compositor.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetTexture)(PP_Resource layer,
                        PP_Resource context,
                        uint32_t target,
                        uint32_t texture,
                        const struct PP_Size* size,
                        struct PP_CompletionCallback cc);
  /**
   * Sets the image of an image layer. If the layer is uninitialized,
   * it will initialize the layer first, and then set its image.
   * The layer size will be set to the image's size. The source rect will be set
   * to the full image. If the layer has been initialized to another kind of
   * layer, the layer will not be changed, and <code>PP_ERROR_BADARGUMENT</code>
   * will be returned.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] image_data A <code>PP_Resource</code> corresponding to
   * an image data resource.
   * param[in] size A <code>PP_Size</code> for the size of the layer before
   * transform. If NULL, the image's size will be used.
   * param[in] cc A <code>PP_CompletionCallback</code> to be called when
   * the image data is released by Chromium compositor.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetImage)(PP_Resource layer,
                      PP_Resource image_data,
                      const struct PP_Size* size,
                      struct PP_CompletionCallback cc);
  /**
   * Sets a clip rectangle for a compositor layer. The Chromium compositor
   * applies a transform matrix on the layer first, and then clips the layer
   * with the rectangle.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] rect The clip rectangle. The origin is top-left corner of
   * the plugin.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetClipRect)(PP_Resource layer, const struct PP_Rect* rect);
  /**
   * Sets a transform matrix which is used to composite the layer.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] matrix A float array with 16 elements. The matrix is
   * column major. The default transform matrix is an identity matrix.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetTransform)(PP_Resource layer, const float matrix[16]);
  /**
   * Sets the opacity value which will be applied to the layer. The effective
   * value of each pixel is computed as:
   *
   *   if (premult_alpha)
   *     pixel.rgb = pixel.rgb * opacity;
   *   pixel.a = pixel.a * opactiy;
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] opacity A <code>float</code> for the opacity value, The default
   * value is 1.0f.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetOpacity)(PP_Resource layer, float opacity);
  /**
   * Sets the blend mode which is used to composite the layer.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] mode A <code>PP_BlendMode</code>. The default mode is
   * <code>PP_BLENDMODE_SRC_OVER</code>.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetBlendMode)(PP_Resource layer, PP_BlendMode mode);
  /**
   * Sets a source rectangle for a texture layer or an image layer.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] rect A <code>PP_FloatRect</code> for an area of the source to
   * consider. For a texture layer, rect is in uv coordinates. For an image
   * layer, rect is in pixels. If the rect is beyond the dimensions of the
   * texture or image, <code>PP_ERROR_BADARGUMENT</code> will be returned.
   * If the layer size does not match the source rect size, bilinear scaling
   * will be used.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetSourceRect)(PP_Resource layer, const struct PP_FloatRect* rect);
  /**
   * Sets the premultiplied alpha for an texture layer.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] premult A <code>PP_Bool</code> with <code>PP_TRUE</code> if
   * pre-multiplied alpha is used.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetPremultipliedAlpha)(PP_Resource layer, PP_Bool premult);
};

struct PPB_CompositorLayer_0_1 { /* dev */
  PP_Bool (*IsCompositorLayer)(PP_Resource resource);
  int32_t (*SetColor)(PP_Resource layer,
                      float red,
                      float green,
                      float blue,
                      float alpha,
                      const struct PP_Size* size);
  int32_t (*SetTexture)(PP_Resource layer,
                        PP_Resource context,
                        uint32_t texture,
                        const struct PP_Size* size,
                        struct PP_CompletionCallback cc);
  int32_t (*SetImage)(PP_Resource layer,
                      PP_Resource image_data,
                      const struct PP_Size* size,
                      struct PP_CompletionCallback cc);
  int32_t (*SetClipRect)(PP_Resource layer, const struct PP_Rect* rect);
  int32_t (*SetTransform)(PP_Resource layer, const float matrix[16]);
  int32_t (*SetOpacity)(PP_Resource layer, float opacity);
  int32_t (*SetBlendMode)(PP_Resource layer, PP_BlendMode mode);
  int32_t (*SetSourceRect)(PP_Resource layer, const struct PP_FloatRect* rect);
  int32_t (*SetPremultipliedAlpha)(PP_Resource layer, PP_Bool premult);
};
/**
 * @}
 */

#endif  /* PPAPI_C_PPB_COMPOSITOR_LAYER_H_ */

