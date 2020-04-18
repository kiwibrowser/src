// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_REFLECTOR_IMPL_H_
#define CONTENT_BROWSER_COMPOSITOR_REFLECTOR_IMPL_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/compositor/image_transport_factory.h"
#include "content/common/content_export.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "ui/compositor/reflector.h"
#include "ui/gfx/geometry/size.h"

namespace gfx { class Rect; }

namespace ui {
class Compositor;
class Layer;
}

namespace content {

class OwnedMailbox;
class BrowserCompositorOutputSurface;

// A reflector implementation that copies the framebuffer content
// to the texture, then draw it onto the mirroring compositor.
class CONTENT_EXPORT ReflectorImpl : public ui::Reflector {
 public:
  ReflectorImpl(ui::Compositor* mirrored_compositor,
                ui::Layer* mirroring_layer);
  ~ReflectorImpl() override;

  ui::Compositor* mirrored_compositor() { return mirrored_compositor_; }

  void Shutdown();

  void DetachFromOutputSurface();

  // ui::Reflector:
  void OnMirroringCompositorResized() override;
  void AddMirroringLayer(ui::Layer* layer) override;
  void RemoveMirroringLayer(ui::Layer* layer) override;

  // Called in |BrowserCompositorOutputSurface::SwapBuffers| to copy
  // the full screen image to the |mailbox_| texture.
  void OnSourceSwapBuffers(const gfx::Size& surface_size);

  // Called in |BrowserCompositorOutputSurface::PostSubBuffer| copy
  // the sub image given by |rect| to the |mailbox_| texture.
  void OnSourcePostSubBuffer(const gfx::Rect& swap_rect,
                             const gfx::Size& surface_size);

  // Called when the source surface is bound and available.
  void OnSourceSurfaceReady(BrowserCompositorOutputSurface* surface);

  // Called when the mailbox which has the source surface's texture
  // is updated.
  void OnSourceTextureMailboxUpdated(scoped_refptr<OwnedMailbox> mailbox);

 private:
  struct LayerData;

  std::vector<std::unique_ptr<LayerData>>::iterator FindLayerData(
      ui::Layer* layer);
  void UpdateTexture(LayerData* layer_data,
                     const gfx::Size& size,
                     const gfx::Rect& redraw_rect);

  ui::Compositor* mirrored_compositor_;
  std::vector<std::unique_ptr<LayerData>> mirroring_layers_;

  scoped_refptr<OwnedMailbox> mailbox_;
  bool flip_texture_;
  BrowserCompositorOutputSurface* output_surface_;

  DISALLOW_COPY_AND_ASSIGN(ReflectorImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_REFLECTOR_IMPL_H_
