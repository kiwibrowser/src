// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_TAB_CAPTURE_TAB_CAPTURE_REGISTRY_H_
#define CHROME_BROWSER_EXTENSIONS_API_TAB_CAPTURE_TAB_CAPTURE_REGISTRY_H_

#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/common/extensions/api/tab_capture.h"
#include "content/public/browser/media_request_state.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_registry_observer.h"

namespace base {
class ListValue;
}

namespace content {
class BrowserContext;
class WebContents;
}

namespace extensions {

class ExtensionRegistry;

namespace tab_capture = api::tab_capture;

class TabCaptureRegistry : public BrowserContextKeyedAPI,
                           public ExtensionRegistryObserver,
                           public MediaCaptureDevicesDispatcher::Observer {
 public:
  static TabCaptureRegistry* Get(content::BrowserContext* context);

  // Used by BrowserContextKeyedAPI.
  static BrowserContextKeyedAPIFactory<TabCaptureRegistry>*
      GetFactoryInstance();

  // List all pending, active and stopped capture requests.
  void GetCapturedTabs(const std::string& extension_id,
                       base::ListValue* list_of_capture_info) const;

  // Add a tab capture request to the registry when a stream is requested
  // through the API.  |target_contents| refers to the WebContents associated
  // with the tab to be captured.  |extension_id| refers to the Extension
  // initiating the request.  |is_anonymous| is true if GetCapturedTabs() should
  // not list the captured tab, and no status change events should be dispatched
  // for it.
  //
  // TODO(miu): This is broken in that it's possible for a later WebContents
  // instance to have the same pointer value as a previously-destroyed one.  To
  // be fixed while working on http://crbug.com/163100.
  bool AddRequest(content::WebContents* target_contents,
                  const std::string& extension_id,
                  bool is_anonymous);

  // Called by MediaStreamDevicesController to verify the request before
  // creating the stream.  |render_process_id| and |render_frame_id| are used to
  // look-up a WebContents instance, which should match the |target_contents|
  // from the prior call to AddRequest().  In addition, a request is not
  // verified unless the |extension_id| also matches AND the request itself is
  // in the PENDING state.
  bool VerifyRequest(int render_process_id,
                     int render_frame_id,
                     const std::string& extension_id);

 private:
  friend class BrowserContextKeyedAPIFactory<TabCaptureRegistry>;
  class LiveRequest;

  explicit TabCaptureRegistry(content::BrowserContext* context);
  ~TabCaptureRegistry() override;

  // Used by BrowserContextKeyedAPI.
  static const char* service_name() {
    return "TabCaptureRegistry";
  }

  static const bool kServiceIsCreatedWithBrowserContext = false;
  static const bool kServiceRedirectedInIncognito = true;

  // ExtensionRegistryObserver implementation.
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;

  // MediaCaptureDevicesDispatcher::Observer implementation.
  void OnRequestUpdate(int original_target_render_process_id,
                       int original_target_render_frame_id,
                       content::MediaStreamType stream_type,
                       const content::MediaRequestState state) override;

  // Send a StatusChanged event containing the current state of |request|.
  void DispatchStatusChangeEvent(const LiveRequest* request) const;

  // Look-up a LiveRequest associated with the given |target_contents| (or
  // the originally targetted RenderFrameHost), if any.
  LiveRequest* FindRequest(const content::WebContents* target_contents) const;
  LiveRequest* FindRequest(int original_target_render_process_id,
                           int original_target_render_frame_id) const;

  // Removes the |request| from |requests_|, thus causing its destruction.
  void KillRequest(LiveRequest* request);

  content::BrowserContext* const browser_context_;
  std::vector<std::unique_ptr<LiveRequest>> requests_;

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;

  DISALLOW_COPY_AND_ASSIGN(TabCaptureRegistry);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_TAB_CAPTURE_TAB_CAPTURE_REGISTRY_H_
