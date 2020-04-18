// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/arcore_device/arcore_gl_thread.h"

#include <utility>
#include "base/message_loop/message_loop.h"
#include "base/version.h"
#include "chrome/browser/android/vr/arcore_device/arcore_gl.h"

namespace device {

ARCoreGlThread::ARCoreGlThread(
    std::unique_ptr<vr::MailboxToSurfaceBridge> mailbox_bridge,
    base::OnceCallback<void(bool)> initialized_callback)
    : base::android::JavaHandlerThread("ARCoreGL"),
      mailbox_bridge_(std::move(mailbox_bridge)),
      initialized_callback_(std::move(initialized_callback)) {}

ARCoreGlThread::~ARCoreGlThread() {
  Stop();
}

ARCoreGl* ARCoreGlThread::GetARCoreGl() {
  return arcore_gl_.get();
}

void ARCoreGlThread::Init() {
  DCHECK(!arcore_gl_);

  arcore_gl_ =
      std::make_unique<ARCoreGl>(base::ResetAndReturn(&mailbox_bridge_));
  bool success = arcore_gl_->Initialize();
  if (!success) {
    CleanUp();
  }

  std::move(initialized_callback_).Run(success);
}

void ARCoreGlThread::CleanUp() {
  arcore_gl_.reset();
}

}  // namespace device
