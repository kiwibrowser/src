/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/device_orientation/device_motion_dispatcher.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/modules/device_orientation/device_motion_controller.h"
#include "third_party/blink/renderer/modules/device_orientation/device_motion_data.h"

namespace blink {

DeviceMotionDispatcher& DeviceMotionDispatcher::Instance() {
  DEFINE_STATIC_LOCAL(DeviceMotionDispatcher, device_motion_dispatcher,
                      (new DeviceMotionDispatcher));
  return device_motion_dispatcher;
}

DeviceMotionDispatcher::DeviceMotionDispatcher() = default;

DeviceMotionDispatcher::~DeviceMotionDispatcher() = default;

void DeviceMotionDispatcher::Trace(blink::Visitor* visitor) {
  visitor->Trace(last_device_motion_data_);
  PlatformEventDispatcher::Trace(visitor);
}

void DeviceMotionDispatcher::StartListening() {
  Platform::Current()->StartListening(kWebPlatformEventTypeDeviceMotion, this);
}

void DeviceMotionDispatcher::StopListening() {
  Platform::Current()->StopListening(kWebPlatformEventTypeDeviceMotion);
  last_device_motion_data_.Clear();
}

void DeviceMotionDispatcher::DidChangeDeviceMotion(
    const device::MotionData& motion) {
  last_device_motion_data_ = DeviceMotionData::Create(motion);
  NotifyControllers();
}

const DeviceMotionData* DeviceMotionDispatcher::LatestDeviceMotionData() {
  return last_device_motion_data_.Get();
}

}  // namespace blink
