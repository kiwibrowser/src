// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_RENDER_FRAME_H_
#define CONTENT_PUBLIC_RENDERER_RENDER_FRAME_H_

#include <stddef.h>

#include <memory>

#include "base/callback_forward.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "content/public/common/console_message_level.h"
#include "content/public/common/previews_state.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"
#include "ppapi/buildflags/buildflags.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/web/web_navigation_policy.h"
#include "third_party/blink/public/web/web_triggering_event_info.h"
#include "ui/accessibility/ax_modes.h"

namespace blink {
class AssociatedInterfaceProvider;
class AssociatedInterfaceRegistry;
class WebFrame;
class WebLocalFrame;
class WebPlugin;
struct WebPluginParams;
}

namespace gfx {
class Range;
class Size;
}

namespace network {
class SharedURLLoaderFactory;
}

namespace service_manager {
class InterfaceProvider;
}

namespace url {
class Origin;
}

namespace content {
class ContextMenuClient;
class PluginInstanceThrottler;
class RenderAccessibility;
class RenderFrameVisitor;
class RenderView;
struct ContextMenuParams;
struct WebPluginInfo;
struct WebPreferences;

// This interface wraps functionality, which is specific to frames, such as
// navigation. It provides communication with a corresponding RenderFrameHost
// in the browser process.
class CONTENT_EXPORT RenderFrame : public IPC::Listener,
                                   public IPC::Sender {
 public:
  // These numeric values are used in UMA logs; do not change them.
  enum PeripheralContentStatus {
    // Content is peripheral, and should be throttled, but is not tiny.
    CONTENT_STATUS_PERIPHERAL = 0,
    // Content is essential because it's same-origin with the top-level frame.
    CONTENT_STATUS_ESSENTIAL_SAME_ORIGIN = 1,
    // Content is essential even though it's cross-origin, because it's large.
    CONTENT_STATUS_ESSENTIAL_CROSS_ORIGIN_BIG = 2,
    // Content is essential because there's large content from the same origin.
    CONTENT_STATUS_ESSENTIAL_CROSS_ORIGIN_WHITELISTED = 3,
    // Content is tiny in size. These are usually blocked.
    CONTENT_STATUS_TINY = 4,
    // Deprecated, as now entirely obscured content is treated as tiny.
    DEPRECATED_CONTENT_STATUS_UNKNOWN_SIZE = 5,
    // Must be last.
    CONTENT_STATUS_NUM_ITEMS
  };

  enum RecordPeripheralDecision {
    DONT_RECORD_DECISION = 0,
    RECORD_DECISION = 1
  };

  // Returns the RenderFrame given a WebLocalFrame.
  static RenderFrame* FromWebFrame(blink::WebLocalFrame* web_frame);

  // Returns the RenderFrame given a routing id.
  static RenderFrame* FromRoutingID(int routing_id);

  // Visit all live RenderFrames.
  static void ForEach(RenderFrameVisitor* visitor);

  // Returns the routing ID for |web_frame|, whether it is a WebLocalFrame in
  // this process or a WebRemoteFrame placeholder for a frame in a different
  // process.
  static int GetRoutingIdForWebFrame(blink::WebFrame* web_frame);

  // Returns the RenderView associated with this frame.
  virtual RenderView* GetRenderView() = 0;

  // Return the RenderAccessibility associated with this frame.
  virtual RenderAccessibility* GetRenderAccessibility() = 0;

  // Get the routing ID of the frame.
  virtual int GetRoutingID() = 0;

  // Returns the associated WebFrame.
  virtual blink::WebLocalFrame* GetWebFrame() = 0;

  // Gets WebKit related preferences associated with this frame.
  virtual const WebPreferences& GetWebkitPreferences() = 0;

  // Shows a context menu with the given information. The given client will
  // be called with the result.
  //
  // The request ID will be returned by this function. This is passed to the
  // client functions for identification.
  //
  // If the client is destroyed, CancelContextMenu() should be called with the
  // request ID returned by this function.
  //
  // Note: if you end up having clients outliving the RenderFrame, we should add
  // a CancelContextMenuCallback function that takes a request id.
  virtual int ShowContextMenu(ContextMenuClient* client,
                              const ContextMenuParams& params) = 0;

  // Cancels a context menu in the event that the client is destroyed before the
  // menu is closed.
  virtual void CancelContextMenu(int request_id) = 0;

  // Create a new Pepper plugin depending on |info|. Returns NULL if no plugin
  // was found. |throttler| may be empty.
  virtual blink::WebPlugin* CreatePlugin(
      const WebPluginInfo& info,
      const blink::WebPluginParams& params,
      std::unique_ptr<PluginInstanceThrottler> throttler) = 0;

  // Execute a string of JavaScript in this frame's context.
  virtual void ExecuteJavaScript(const base::string16& javascript) = 0;

  // Returns true if this is the main (top-level) frame.
  virtual bool IsMainFrame() = 0;

  // Return true if this frame is hidden.
  virtual bool IsHidden() = 0;

  // Ask the RenderFrame (or its observers) to bind a request for
  // |interface_name| to |interface_pipe|.
  virtual void BindLocalInterface(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe) = 0;

  // Returns the InterfaceProvider that this process can use to bind
  // interfaces exposed to it by the application running in this frame.
  virtual service_manager::InterfaceProvider* GetRemoteInterfaces() = 0;

  // Returns the AssociatedInterfaceRegistry this frame can use to expose
  // frame-specific Channel-associated interfaces to the remote RenderFrameHost.
  virtual blink::AssociatedInterfaceRegistry*
  GetAssociatedInterfaceRegistry() = 0;

  // Returns the AssociatedInterfaceProvider this frame can use to access
  // frame-specific Channel-associated interfaces from the remote
  // RenderFrameHost.
  virtual blink::AssociatedInterfaceProvider*
  GetRemoteAssociatedInterfaces() = 0;

#if BUILDFLAG(ENABLE_PLUGINS)
  // Registers a plugin that has been marked peripheral. If the origin
  // whitelist is later updated and includes |content_origin|, then
  // |unthrottle_callback| will be called.
  virtual void RegisterPeripheralPlugin(
      const url::Origin& content_origin,
      const base::Closure& unthrottle_callback) = 0;

  // Returns the peripheral content heuristic decision.
  //
  // Power Saver is enabled for plugin content that are cross-origin and
  // heuristically determined to be not essential to the web page content.
  //
  // Plugin content is defined to be cross-origin when the plugin source's
  // origin differs from the top level frame's origin. For example:
  //  - Cross-origin:  a.com -> b.com/plugin.swf
  //  - Cross-origin:  a.com -> b.com/iframe.html -> b.com/plugin.swf
  //  - Same-origin:   a.com -> b.com/iframe-to-a.html -> a.com/plugin.swf
  //
  // |main_frame_origin| is the origin of the main frame.
  //
  // |content_origin| is the origin of the plugin content.
  //
  // |unobscured_size| are zoom and device scale independent logical pixels.
  virtual PeripheralContentStatus GetPeripheralContentStatus(
      const url::Origin& main_frame_origin,
      const url::Origin& content_origin,
      const gfx::Size& unobscured_size,
      RecordPeripheralDecision record_decision) const = 0;

  // Whitelists a |content_origin| so its content will never be throttled in
  // this RenderFrame. Whitelist is cleared by top level navigation.
  virtual void WhitelistContentOrigin(const url::Origin& content_origin) = 0;

  // Used by plugins that load data in this RenderFrame to update the loading
  // notifications.
  virtual void PluginDidStartLoading() = 0;
  virtual void PluginDidStopLoading() = 0;
#endif

  // Returns true if this frame is a FTP directory listing.
  virtual bool IsFTPDirectoryListing() = 0;

  // Attaches the browser plugin identified by |element_instance_id| to guest
  // content created by the embedder.
  virtual void AttachGuest(int element_instance_id) = 0;

  // Detaches the browser plugin identified by |element_instance_id| from guest
  // content created by the embedder.
  virtual void DetachGuest(int element_instance_id) = 0;

  // Notifies the browser of text selection changes made.
  virtual void SetSelectedText(const base::string16& selection_text,
                               size_t offset,
                               const gfx::Range& range) = 0;

  // Adds |message| to the DevTools console.
  virtual void AddMessageToConsole(ConsoleMessageLevel level,
                                   const std::string& message) = 0;

  // Sets the PreviewsState of this frame, a bitmask of potentially several
  // Previews optimizations.
  virtual void SetPreviewsState(PreviewsState previews_state) = 0;

  // Returns the PreviewsState of this frame, a bitmask of potentially several
  // Previews optimizations.
  virtual PreviewsState GetPreviewsState() const = 0;

  // Whether or not this frame is currently pasting.
  virtual bool IsPasting() const = 0;

  // Returns the current visibility of the frame.
  virtual blink::mojom::PageVisibilityState GetVisibilityState() const = 0;

  // If PlzNavigate is enabled, returns true in between teh time that Blink
  // requests navigation until the browser responds with the result.
  virtual bool IsBrowserSideNavigationPending() = 0;

  // Renderer scheduler frame-specific task queues handles.
  // See third_party/WebKit/Source/platform/WebFrameScheduler.h for details.
  virtual scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(
      blink::TaskType task_type) = 0;

  // Bitwise-ORed set of extra bindings that have been enabled.  See
  // BindingsPolicy for details.
  virtual int GetEnabledBindings() const = 0;

  // Set the accessibility mode to force creation of RenderAccessibility.
  virtual void SetAccessibilityModeForTest(ui::AXMode new_mode) = 0;

  virtual scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactory() = 0;

 protected:
  ~RenderFrame() override {}

 private:
  // This interface should only be implemented inside content.
  friend class RenderFrameImpl;
  RenderFrame() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_RENDER_FRAME_H_
