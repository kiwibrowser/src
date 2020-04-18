// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/plugins/navigator_plugins.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/modules/plugins/dom_mime_type_array.h"
#include "third_party/blink/renderer/modules/plugins/dom_plugin_array.h"

namespace blink {

NavigatorPlugins::NavigatorPlugins(Navigator& navigator)
    : Supplement<Navigator>(navigator) {}

// static
NavigatorPlugins& NavigatorPlugins::From(Navigator& navigator) {
  NavigatorPlugins* supplement = ToNavigatorPlugins(navigator);
  if (!supplement) {
    supplement = new NavigatorPlugins(navigator);
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

// static
NavigatorPlugins* NavigatorPlugins::ToNavigatorPlugins(Navigator& navigator) {
  return Supplement<Navigator>::From<NavigatorPlugins>(navigator);
}

// static
const char NavigatorPlugins::kSupplementName[] = "NavigatorPlugins";

// static
DOMPluginArray* NavigatorPlugins::plugins(Navigator& navigator) {
  return NavigatorPlugins::From(navigator).plugins(navigator.GetFrame());
}

// static
DOMMimeTypeArray* NavigatorPlugins::mimeTypes(Navigator& navigator) {
  return NavigatorPlugins::From(navigator).mimeTypes(navigator.GetFrame());
}

// static
bool NavigatorPlugins::javaEnabled(Navigator& navigator) {
  return false;
}

DOMPluginArray* NavigatorPlugins::plugins(LocalFrame* frame) const {
  if (!plugins_)
    plugins_ = DOMPluginArray::Create(frame);
  return plugins_.Get();
}

DOMMimeTypeArray* NavigatorPlugins::mimeTypes(LocalFrame* frame) const {
  if (!mime_types_)
    mime_types_ = DOMMimeTypeArray::Create(frame);
  return mime_types_.Get();
}

void NavigatorPlugins::Trace(blink::Visitor* visitor) {
  visitor->Trace(plugins_);
  visitor->Trace(mime_types_);
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
