// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_BROWSER_PLUGIN_DELEGATE_H_
#define CONTENT_PUBLIC_RENDERER_BROWSER_PLUGIN_DELEGATE_H_

#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"

namespace gfx {
class Size;
}

namespace v8 {
class Isolate;
class Object;
template<typename T> class Local;
}  // namespace v8

namespace content {

// A delegate for BrowserPlugin which gets notified about the plugin load.
// Implementations can provide additional steps necessary to change the load
// behavior of the plugin.
class CONTENT_EXPORT BrowserPluginDelegate {
 public:
  // Called when the BrowserPlugin's geometry has been computed for the first
  // time.
  virtual void Ready() {}

  // Called when plugin document has finished loading.
  virtual void PluginDidFinishLoading() {}

  // Called when plugin document receives data.
  virtual void PluginDidReceiveData(const char* data, int data_length) {}

  // Sets the instance ID that idenfies the plugin within current render
  // process.
  virtual void SetElementInstanceID(int element_instance_id) {}

  // Called when the plugin resizes.
  virtual void DidResizeElement(const gfx::Size& new_size) {}

  // Called when the plugin is about to be destroyed.
  virtual void DidDestroyElement() {}

  // Returns a scriptable object for the plugin.
  virtual v8::Local<v8::Object> V8ScriptableObject(v8::Isolate* isolate);

  // Returns a weak pointer to this delegate.
  virtual base::WeakPtr<BrowserPluginDelegate> GetWeakPtr() = 0;

 protected:
  virtual ~BrowserPluginDelegate() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_BROWSER_PLUGIN_DELEGATE_H_
