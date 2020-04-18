// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_COMPOSITOR_LAYER_H_
#define PPAPI_CPP_COMPOSITOR_LAYER_H_

#include <stdint.h>

#include "ppapi/c/ppb_compositor_layer.h"
#include "ppapi/cpp/graphics_3d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/size.h"

namespace pp {

class CompositorLayer : public Resource {
 public:
  /// Default constructor for creating an is_null()
  /// <code>CompositorLayer</code> object.
  CompositorLayer();

  /// The copy constructor for <code>CompositorLayer</code>.
  ///
  /// @param[in] other A reference to a <code>CompositorLayer</code>.
  CompositorLayer(const CompositorLayer& other);

  /// Constructs a <code>CompositorLayer</code> from a <code>Resource</code>.
  ///
  /// @param[in] resource A <code>PPB_CompositorLayer</code> resource.
  explicit CompositorLayer(const Resource& resource);

  /// A constructor used when you have received a <code>PP_Resource</code> as a
  /// return value that has had 1 ref added for you.
  ///
  /// @param[in] resource A <code>PPB_CompositorLayer</code> resource.
  CompositorLayer(PassRef, PP_Resource resource);

  /// Destructor.
  ~CompositorLayer();

  /// Sets the color of a solid color layer. If the layer is uninitialized, it
  /// will initialize the layer first, and then set its color. If the layer has
  /// been initialized to another kind of layer, the layer will not be changed,
  /// and <code>PP_ERROR_BADARGUMENT</code> will be returned.
  ///
  /// param[in] red A <code>float</code> for the red color component. It will be
  /// clamped to [0, 1]
  /// param[in] green A <code>float</code> for the green color component.
  /// It will be clamped to [0, 1].
  /// param[in] blue A <code>float</code> for the blue color component. It will
  /// be clamped to [0, 1].
  /// param[in] alpha A <code>float</code> for the alpha color component.
  /// It will be clamped to [0, 1].
  /// param[in] size A <code>Size</code> for the size of the layer before
  /// transform.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetColor(float red,
                   float green,
                   float blue,
                   float alpha,
                   const Size& size);

  /// Sets the texture of a texture layer. If the layer is uninitialized, it
  /// will initialize the layer first, and then set its texture. The source rect
  /// will be set to ((0, 0), (1, 1)). If the layer has been initialized to
  /// another kind of layer, the layer will not be changed, and
  /// <code>PP_ERROR_BADARGUMENT</code> will be returned.
  ///
  /// param[in] context A <code>Graphics3D</code> corresponding to a graphics 3d
  /// resource which owns the GL texture.
  /// param[in] target GL texture target (GL_TEXTURE_2D, etc).
  /// param[in] texture A GL texture object id.
  /// param[in] size A <code>Size</code> for the size of the layer before
  /// transform.
  /// param[in] cc A <code>CompletionCallback</code> to be called when
  /// the texture is released by Chromium compositor.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetTexture(const Graphics3D& context,
                     uint32_t target,
                     uint32_t texture,
                     const Size& size,
                     const CompletionCallback& cc);

  /// Sets the image of an image layer. If the layer is uninitialized, it will
  /// initiliaze the layer first, and then set the image of it. If the layer has
  /// been initialized to another kind of layer, the layer will not be changed,
  /// and <code>PP_ERROR_BADARGUMENT</code> will be returned.
  ///
  /// param[in] image_data A <code>PP_Resource</code> corresponding to an image
  /// data resource.
  /// param[in] cc A <code>CompletionCallback</code> to be called when
  /// the image data is released by Chromium compositor.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetImage(const ImageData& image,
                   const CompletionCallback& callback);

  /// Sets the image of an image layer. If the layer is uninitialized, it will
  /// initialize the layer first, and then set its image. The layer size will
  /// be set to the image's size. The source rect will be set to the full image.
  /// If the layer has been initialized to another kind of layer, the layer will
  /// not be changed, and <code>PP_ERROR_BADARGUMENT</code> will be returned.
  ///
  /// param[in] image_data A <code>ImageData</code> corresponding to an image
  /// data resource.
  /// param[in] size A <code>Size</code> for the size of the layer before
  /// transform.
  /// param[in] cc A <code>CompletionCallback</code> to be called when the image
  /// data is released by Chromium compositor.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetImage(const ImageData& image,
                   const Size& size,
                   const CompletionCallback& callback);

  /// Sets a clip rectangle for a compositor layer. The Chromium compositor
  /// applies a transform matrix on the layer first, and then clips the layer
  /// with the rectangle.
  ///
  /// param[in] rect The clip rectangle. The origin is top-left corner of
  /// the plugin.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetClipRect(const Rect& rect);

  /// Sets a transform matrix which is used to composite the layer.
  ///
  /// param[in] matrix A float array with 16 elements. The matrix is coloum
  /// major. The default transform matrix is an identity matrix.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetTransform(const float matrix[16]);

  /// Sets the opacity value which will be applied to the layer. The effective
  /// value of each pixel is computed as:
  ///
  ///   if (premult_alpha)
  ///     pixel.rgb = pixel.rgb * opacity;
  ///   pixel.a = pixel.a * opactiy;
  ///
  /// param[in] opacity A <code>float</code> for the opacity value.
  /// The default value is 1.0f.
  ///
  ///@return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetOpacity(float opacity);

  /// Sets the blend mode which is used to composite the layer.
  ///
  /// param[in] mode A <code>PP_BlendMode</code>. The default value is
  /// <code>PP_BLENDMODE_SRC_OVER</code>.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetBlendMode(PP_BlendMode mode);

  /// Sets a source rectangle for a texture layer or an image layer.
  ///
  /// param[in] rect A <code>FloatRect</code> for an area of the source to
  /// consider. For a texture layer, rect is in uv coordinates. For an image
  /// layer, rect is in pixels. If the rect is beyond the dimensions of the
  /// texture or image, <code>PP_ERROR_BADARGUMENT</code> will be returned.
  /// If the layer size does not match the source rect size, bilinear scaling
  /// will be used.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetSourceRect(const FloatRect& rect);

  /// Sets the premultiplied alpha for an texture layer.
  ///
  /// param[in] premult A <code>bool</code> with <code>true</code> if
  /// pre-multiplied alpha is used.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t SetPremultipliedAlpha(bool premult);

  /// Checks whether a <code>Resource</code> is a compositor layer, to test
  /// whether it is appropriate for use with the <code>CompositorLayer</code>
  /// constructor.
  ///
  /// @param[in] resource A <code>Resource</code> to test.
  ///
  /// @return True if <code>resource</code> is a compositor layer.
  static bool IsCompositorLayer(const Resource& resource);
};

}  // namespace pp

#endif  // PPAPI_CPP_COMPOSITOR_LAYER_H_
