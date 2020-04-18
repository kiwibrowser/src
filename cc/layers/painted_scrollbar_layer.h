// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_PAINTED_SCROLLBAR_LAYER_H_
#define CC_LAYERS_PAINTED_SCROLLBAR_LAYER_H_

#include "base/macros.h"
#include "cc/cc_export.h"
#include "cc/input/scrollbar.h"
#include "cc/layers/layer.h"
#include "cc/layers/scrollbar_layer_interface.h"
#include "cc/layers/scrollbar_theme_painter.h"
#include "cc/resources/scoped_ui_resource.h"

namespace cc {

class CC_EXPORT PaintedScrollbarLayer : public ScrollbarLayerInterface,
                                        public Layer {
 public:
  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;

  static scoped_refptr<PaintedScrollbarLayer> Create(
      std::unique_ptr<Scrollbar> scrollbar,
      ElementId element_id = ElementId());

  bool OpacityCanAnimateOnImplThread() const override;
  ScrollbarLayerInterface* ToScrollbarLayer() override;

  // ScrollbarLayerInterface
  void SetScrollElementId(ElementId element_id) override;

  // Layer interface
  bool Update() override;
  void SetLayerTreeHost(LayerTreeHost* host) override;
  void PushPropertiesTo(LayerImpl* layer) override;

  const gfx::Size& internal_content_bounds() const {
    return internal_content_bounds_;
  }

 protected:
  PaintedScrollbarLayer(std::unique_ptr<Scrollbar> scrollbar,
                        ElementId scroll_element_id);
  ~PaintedScrollbarLayer() override;

  // For unit tests
  UIResourceId track_resource_id() {
    return track_resource_.get() ? track_resource_->id() : 0;
  }
  UIResourceId thumb_resource_id() {
    return thumb_resource_.get() ? thumb_resource_->id() : 0;
  }
  void UpdateInternalContentScale();
  void UpdateThumbAndTrackGeometry();

 private:
  gfx::Rect ScrollbarLayerRectToContentRect(const gfx::Rect& layer_rect) const;
  gfx::Rect OriginThumbRect() const;

  template <typename T>
  bool UpdateProperty(T value, T* prop) {
    if (*prop == value)
      return false;
    *prop = value;
    SetNeedsPushProperties();
    return true;
  }

  UIResourceBitmap RasterizeScrollbarPart(const gfx::Rect& layer_rect,
                                          const gfx::Rect& content_rect,
                                          ScrollbarPart part);

  std::unique_ptr<Scrollbar> scrollbar_;
  ElementId scroll_element_id_;

  float internal_contents_scale_;
  gfx::Size internal_content_bounds_;

  // Snapshot of properties taken in UpdateThumbAndTrackGeometry and used in
  // PushPropertiesTo.
  int thumb_thickness_;
  int thumb_length_;
  gfx::Point location_;
  gfx::Rect track_rect_;
  bool is_overlay_;
  bool has_thumb_;

  std::unique_ptr<ScopedUIResource> track_resource_;
  std::unique_ptr<ScopedUIResource> thumb_resource_;

  float thumb_opacity_;

  DISALLOW_COPY_AND_ASSIGN(PaintedScrollbarLayer);
};

}  // namespace cc

#endif  // CC_LAYERS_PAINTED_SCROLLBAR_LAYER_H_
