// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_APP_WEB_MESSAGE_PORT_H_
#define CONTENT_BROWSER_ANDROID_APP_WEB_MESSAGE_PORT_H_

#include "base/android/jni_weak_ref.h"
#include "third_party/blink/public/common/message_port/message_port_channel.h"

namespace content {

namespace AppWebMessagePort {

std::vector<blink::MessagePortChannel> UnwrapJavaArray(
    JNIEnv* env,
    const base::android::JavaRef<jobjectArray>& jports);

}  // namespace AppWebMessagePort
}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_APP_WEB_MESSAGE_PORT_H_
