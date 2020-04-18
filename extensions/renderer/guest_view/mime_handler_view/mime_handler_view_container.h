// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_GUEST_VIEW_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_CONTAINER_H_
#define EXTENSIONS_RENDERER_GUEST_VIEW_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_CONTAINER_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "components/guest_view/renderer/guest_view_container.h"
#include "content/public/common/transferrable_url_loader.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "third_party/blink/public/web/web_associated_url_loader_client.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace blink {
class WebAssociatedURLLoader;
}  // namespace blink

namespace content {
class URLLoaderThrottle;
struct WebPluginInfo;
}  // namespace content

namespace extensions {

// A container for loading up an extension inside a BrowserPlugin to handle a
// MIME type. A request for the URL of the data to load inside the container is
// made and a url is sent back in response which points to the URL which the
// container should be navigated to. There are two cases for making this URL
// request, the case where the plugin is embedded and the case where it is top
// level:
// 1) In the top level case a URL request for the data to load has already been
//    made by the renderer on behalf of the plugin. The |DidReceiveData| and
//    |DidFinishLoading| callbacks (from BrowserPluginDelegate) will be called
//    when data is received and when it has finished being received,
//    respectively.
// 2) In the embedded case, no URL request is automatically made by the
//    renderer. We make a URL request for the data inside the container using
//    a WebAssociatedURLLoader. In this case, the |didReceiveData| and
//    |didFinishLoading| (from WebAssociatedURLLoaderClient) when data is
//    received and when it has finished being received.
class MimeHandlerViewContainer : public guest_view::GuestViewContainer,
                                 public blink::WebAssociatedURLLoaderClient {
 public:
  MimeHandlerViewContainer(content::RenderFrame* render_frame,
                           const content::WebPluginInfo& info,
                           const std::string& mime_type,
                           const GURL& original_url);

  static std::vector<MimeHandlerViewContainer*> FromRenderFrame(
      content::RenderFrame* render_frame);

  // GuestViewContainer implementation.
  bool OnMessage(const IPC::Message& message) override;
  void OnReady() override;

  // BrowserPluginDelegate implementation.
  void PluginDidFinishLoading() override;
  void PluginDidReceiveData(const char* data, int data_length) override;
  void DidResizeElement(const gfx::Size& new_size) override;
  v8::Local<v8::Object> V8ScriptableObject(v8::Isolate*) override;

  // WebAssociatedURLLoaderClient overrides.
  void DidReceiveData(const char* data, int data_length) override;
  void DidFinishLoading() override;

  // GuestViewContainer overrides.
  void OnRenderFrameDestroyed() override;

  // Post a JavaScript message to the guest.
  void PostJavaScriptMessage(v8::Isolate* isolate,
                             v8::Local<v8::Value> message);

  // Post |message| to the guest.
  void PostMessageFromValue(const base::Value& message);

  // If the URL matches the same URL that this object has created and it hasn't
  // added a throttle yet, it will return a new one for the purpose of
  // intercepting it.
  std::unique_ptr<content::URLLoaderThrottle> MaybeCreatePluginThrottle(
      const GURL& url);

 protected:
  ~MimeHandlerViewContainer() override;

 private:
  class PluginResourceThrottle;

  // Called for embedded plugins when network service is enabled. This is called
  // by the URLLoaderThrottle which intercepts the resource load, which is then
  // sent to the browser to be handed off to the plugin.
  void SetEmbeddedLoader(
      content::mojom::TransferrableURLLoaderPtr transferrable_url_loader);

  // Message handlers.
  void OnCreateMimeHandlerViewGuestACK(int element_instance_id);
  void OnGuestAttached(int element_instance_id,
                       int guest_proxy_routing_id);
  void OnMimeHandlerViewGuestOnLoadCompleted(int element_instance_id);

  // Creates a guest when a geometry and the URL of the extension to navigate
  // to are available.
  void CreateMimeHandlerViewGuestIfNecessary();

  // Path of the plugin.
  const std::string plugin_path_;

  // The MIME type of the plugin.
  const std::string mime_type_;

  // The URL of the extension to navigate to.
  std::string view_id_;

  // Whether the plugin is embedded or not.
  bool is_embedded_;

  // The original URL of the plugin.
  GURL original_url_;

  // The RenderView routing ID of the guest.
  int guest_proxy_routing_id_;

  // Used when network service is enabled:
  bool waiting_to_create_throttle_ = false;
  content::mojom::TransferrableURLLoaderPtr transferrable_url_loader_;

  // Used when network service is disabled:
  // A URL loader to load the |original_url_| when the plugin is embedded. In
  // the embedded case, no URL request is made automatically.
  std::unique_ptr<blink::WebAssociatedURLLoader> loader_;

  // The scriptable object that backs the plugin.
  v8::Global<v8::Object> scriptable_object_;

  // Pending postMessage messages that need to be sent to the guest. These are
  // queued while the guest is loading and once it is fully loaded they are
  // delivered so that messages aren't lost.
  std::vector<v8::Global<v8::Value>> pending_messages_;

  // True if a guest process has been requested.
  bool guest_created_ = false;

  // True if the guest page has fully loaded and its JavaScript onload function
  // has been called.
  bool guest_loaded_;

  // The size of the element.
  base::Optional<gfx::Size> element_size_;

  base::WeakPtrFactory<MimeHandlerViewContainer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewContainer);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_GUEST_VIEW_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_CONTAINER_H_
