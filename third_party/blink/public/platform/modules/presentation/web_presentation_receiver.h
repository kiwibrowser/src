// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PRESENTATION_WEB_PRESENTATION_RECEIVER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PRESENTATION_WEB_PRESENTATION_RECEIVER_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

// The delegate Blink provides to WebPresentationClient in order to get
// notified when to initialize the Presentation Receiver API (i.e. when
// frame is finished loading) or when to terminate all receiver
// PresentationConnections.
// TODO(crbug.com/749327): Remove this interface as part of onion soup. Blink
// should be able to directly listen for the notifications that are used to call
// these methods.
class BLINK_PLATFORM_EXPORT WebPresentationReceiver {
 public:
  virtual ~WebPresentationReceiver() = default;

  // Initializes the PresentationReceiver object, such as setting up Mojo
  // connections. No-ops if already initialized.
  virtual void Init() = 0;

  // Notifies the PresentationReceiver that it is about to be terminated (e.g.,
  // due to frame closing).
  virtual void OnReceiverTerminated() = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PRESENTATION_WEB_PRESENTATION_RECEIVER_H_
