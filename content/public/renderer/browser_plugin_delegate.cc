// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/browser_plugin_delegate.h"

#include "v8/include/v8.h"

namespace content {

v8::Local<v8::Object> BrowserPluginDelegate::V8ScriptableObject(
    v8::Isolate* isolate) {
  return v8::Local<v8::Object>();
}

}  // namespace content
