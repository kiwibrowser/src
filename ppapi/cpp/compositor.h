// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_COMPOSITOR_H_
#define PPAPI_CPP_COMPOSITOR_H_

#include <stdint.h>

#include "ppapi/c/ppb_compositor.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/compositor_layer.h"
#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the API to create a compositor in the browser.
namespace pp {

/// The <code>Compositor</code> interface is used for setting
/// <code>CompositorLayer</code> layers to the Chromium compositor for
/// compositing. This allows a plugin to combine different sources of visual
/// data efficiently, such as <code>ImageData</code> images and OpenGL textures.
/// See also <code>CompositorLayer</code> for more information.
class Compositor : public Resource {
 public:
  /// Default constructor for creating an is_null()
  /// <code>Compositor</code> object.
  Compositor();

  /// A constructor for creating and initializing a compositor.
  ///
  /// On failure, the object will be is_null().
  explicit Compositor(const InstanceHandle& instance);

  /// The copy constructor for <code>Compositor</code>.
  ///
  /// @param[in] other A reference to a <code>Compositor</code>.
  Compositor(const Compositor& other);

  /// Constructs a <code>Compositor</code> from a <code>Resource</code>.
  ///
  /// @param[in] resource A <code>PPB_Compositor</code> resource.
  explicit Compositor(const Resource& resource);

  /// A constructor used when you have received a <code>PP_Resource</code> as a
  /// return value that has had 1 ref added on behalf of the caller.
  ///
  /// @param[in] resource A <code>PPB_Compositor</code> resource.
  Compositor(PassRef, PP_Resource resource);

  /// Destructor.
  ~Compositor();

  /// Creates a new <code>CompositorLayer</code> and adds it to the end of the
  /// layer stack. A <code>CompositorLayer</code> containing the layer is
  /// returned. It is uninitialized, <code>SetColor()</code>,
  /// <code>SetTexture</code> or <code>SetImage</code> should be used to
  /// initialize it. The layer will appear above other pre-existing layers.
  /// If <code>ResetLayers</code> is called or the <code>PPB_Compositor</code>
  /// is released, the returned layer will be invalidated, and any further calls
  /// on the layer will return <code>PP_ERROR_BADRESOURCE</code>.
  ///
  /// @return A <code>CompositorLayer</code> containing the compositor layer
  /// resource.
  CompositorLayer AddLayer();

  /// Commits layers added by <code>AddLayer()</code> to the chromium
  /// compositor.
  ///
  /// @param[in] cc A <code>CompletionCallback</code> to be called when
  /// layers have been represented on screen.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t CommitLayers(const CompletionCallback& cc);

  /// Resets layers added by <code>AddLayer()</code>
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  int32_t ResetLayers();

  /// Checks whether a <code>Resource</code> is a compositor, to test whether
  /// it is appropriate for use with the <code>Compositor</code> constructor.
  ///
  /// @param[in] resource A <code>Resource</code> to test.
  ///
  /// @return True if <code>resource</code> is a compositor.
  static bool IsCompositor(const Resource& resource);
};

}  // namespace pp

#endif  // PPAPI_CPP_COMPOSITOR_H_
