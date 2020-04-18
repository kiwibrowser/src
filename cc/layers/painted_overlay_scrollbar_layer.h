// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_PAINTED_OVERLAY_SCROLLBAR_LAYER_H_
#define CC_LAYERS_PAINTED_OVERLAY_SCROLLBAR_LAYER_H_

#include "base/macros.h"
#include "cc/cc_export.h"
#include "cc/input/scrollbar.h"
#include "cc/layers/layer.h"
#include "cc/layers/scrollbar_layer_interface.h"
#include "cc/layers/scrollbar_theme_painter.h"
#include "cc/resources/scoped_ui_resource.h"

namespace cc {

class CC_EXPORT PaintedOverlayScrollbarLayer : public ScrollbarLayerInterface,
                                               public Layer {
 public:
  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;

  static scoped_refptr<PaintedOverlayScrollbarLayer> Create(
      std::unique_ptr<Scrollbar> scrollbar,
      ElementId scroll_element_id = ElementId());

  bool OpacityCanAnimateOnImplThread() const override;
  ScrollbarLayerInterface* ToScrollbarLayer() override;

  // ScrollbarLayerInterface
  void SetScrollElementId(ElementId element_id) override;

  // Layer interface
  bool Update() override;
  void SetLayerTreeHost(LayerTreeHost* host) override;
  void PushPropertiesTo(LayerImpl* layer) override;

 protected:
  PaintedOverlayScrollbarLayer(std::unique_ptr<Scrollbar> scrollbar,
                               ElementId scroll_element_id);
  ~PaintedOverlayScrollbarLayer() override;

 private:
  gfx::Rect OriginThumbRectForPainting() const;

  template <typename T>
  bool UpdateProperty(T value, T* prop) {
    if (*prop == value)
      return false;
    *prop = value;
    SetNeedsPushProperties();
    return true;
  }

  bool PaintThumbIfNeeded();
  bool PaintTickmarks();

  std::unique_ptr<Scrollbar> scrollbar_;
  ElementId scroll_element_id_;

  int thumb_thickness_;
  int thumb_length_;
  gfx::Point location_;
  gfx::Rect track_rect_;

  gfx::Rect aperture_;

  std::unique_ptr<ScopedUIResource> thumb_resource_;
  std::unique_ptr<ScopedUIResource> track_resource_;

  DISALLOW_COPY_AND_ASSIGN(PaintedOverlayScrollbarLayer);
};

}  // namespace cc

#endif  // CC_LAYERS_PAINTED_OVERLAY_SCROLLBAR_LAYER_H_
