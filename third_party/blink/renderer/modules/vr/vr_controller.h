// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_CONTROLLER_H_

#include "device/vr/public/mojom/vr_service.mojom-blink.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/modules/vr/vr_display.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"

#include <memory>

namespace blink {

class NavigatorVR;
class VRGetDevicesCallback;

class VRController final : public GarbageCollectedFinalized<VRController>,
                           public device::mojom::blink::VRServiceClient,
                           public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(VRController);
  WTF_MAKE_NONCOPYABLE(VRController);
  USING_PRE_FINALIZER(VRController, Dispose);

 public:
  VRController(NavigatorVR*);
  ~VRController() override;

  void GetDisplays(ScriptPromiseResolver*);
  void SetListeningForActivate(bool);

  void OnDisplayConnected(device::mojom::blink::VRMagicWindowProviderPtr,
                          device::mojom::blink::VRDisplayHostPtr,
                          device::mojom::blink::VRDisplayClientRequest,
                          device::mojom::blink::VRDisplayInfoPtr) override;

  void FocusChanged();

  void Trace(blink::Visitor*) override;

 private:
  void OnDisplaysSynced();
  void OnGetDisplays();

  // ContextLifecycleObserver.
  void ContextDestroyed(ExecutionContext*) override;
  void Dispose();

  void LogGetDisplayResult();

  Member<NavigatorVR> navigator_vr_;
  VRDisplayVector displays_;

  bool display_synced_;

  bool has_presentation_capable_display_ = false;
  bool has_display_ = false;

  Deque<std::unique_ptr<VRGetDevicesCallback>> pending_get_devices_callbacks_;
  device::mojom::blink::VRServicePtr service_;
  mojo::Binding<device::mojom::blink::VRServiceClient> binding_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_CONTROLLER_H_
