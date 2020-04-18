// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_PRESENTATION_CONNECTION_CALLBACKS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_PRESENTATION_CONNECTION_CALLBACKS_H_

#include "third_party/blink/public/platform/modules/presentation/presentation.mojom-blink.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class ControllerPresentationConnection;
class PresentationRequest;
class ScriptPromiseResolver;

// PresentationConnectionCallbacks resolves or rejects the provided resolver's
// underlying promise depending on the result passed to the callback. On
// success, the promise will be resolved with a newly created
// ControllerPresentationConnection. In the case of reconnect, the callback may
// take an existing connection object with which the promise will be resolved
// on success.
class PresentationConnectionCallbacks final {
 public:
  PresentationConnectionCallbacks(ScriptPromiseResolver*, PresentationRequest*);
  PresentationConnectionCallbacks(ScriptPromiseResolver*,
                                  ControllerPresentationConnection*);
  ~PresentationConnectionCallbacks() = default;

  void HandlePresentationResponse(mojom::blink::PresentationInfoPtr,
                                  mojom::blink::PresentationErrorPtr);

 private:
  void OnSuccess(const mojom::blink::PresentationInfo&);
  void OnError(const mojom::blink::PresentationError&);

  Persistent<ScriptPromiseResolver> resolver_;
  Persistent<PresentationRequest> request_;
  WeakPersistent<ControllerPresentationConnection> connection_;

  WTF_MAKE_NONCOPYABLE(PresentationConnectionCallbacks);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_PRESENTATION_CONNECTION_CALLBACKS_H_
