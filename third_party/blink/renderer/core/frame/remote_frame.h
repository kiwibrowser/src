// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REMOTE_FRAME_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REMOTE_FRAME_H_

#include "third_party/blink/public/platform/web_focus_type.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/execution_context/remote_security_context.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/frame/remote_frame_view.h"

namespace cc {
class Layer;
}

namespace blink {

class LocalFrame;
class RemoteFrameClient;
struct FrameLoadRequest;

class CORE_EXPORT RemoteFrame final : public Frame {
 public:
  static RemoteFrame* Create(RemoteFrameClient*, Page&, FrameOwner*);

  ~RemoteFrame() override;

  // Frame overrides:
  void Trace(blink::Visitor*) override;
  void Navigate(Document& origin_document,
                const KURL&,
                bool replace_current_item,
                UserGestureStatus) override;
  void Navigate(const FrameLoadRequest& passed_request) override;
  void Reload(FrameLoadType, ClientRedirectPolicy) override;
  void Detach(FrameDetachType) override;
  RemoteSecurityContext* GetSecurityContext() const override;
  bool PrepareForCommit() override;
  void CheckCompleted() override;
  bool ShouldClose() override;
  void DidFreeze() override;
  void DidResume() override;
  void SetIsInert(bool) override;
  void SetInheritedEffectiveTouchAction(TouchAction) override;

  void SetCcLayer(cc::Layer*, bool prevent_contents_opaque_changes);
  cc::Layer* GetCcLayer() const { return cc_layer_; }
  bool WebLayerHasFixedContentsOpaque() const {
    return prevent_contents_opaque_changes_;
  }

  void AdvanceFocus(WebFocusType, LocalFrame* source);

  void SetView(RemoteFrameView*);
  void CreateView();

  RemoteFrameView* View() const override;

  RemoteFrameClient* Client() const;

 private:
  RemoteFrame(RemoteFrameClient*, Page&, FrameOwner*);

  // Intentionally private to prevent redundant checks when the type is
  // already RemoteFrame.
  bool IsLocalFrame() const override { return false; }
  bool IsRemoteFrame() const override { return true; }

  void DetachChildren();

  Member<RemoteFrameView> view_;
  Member<RemoteSecurityContext> security_context_;
  cc::Layer* cc_layer_ = nullptr;
  bool prevent_contents_opaque_changes_ = false;
};

inline RemoteFrameView* RemoteFrame::View() const {
  return view_.Get();
}

DEFINE_TYPE_CASTS(RemoteFrame,
                  Frame,
                  remoteFrame,
                  remoteFrame->IsRemoteFrame(),
                  remoteFrame.IsRemoteFrame());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REMOTE_FRAME_H_
