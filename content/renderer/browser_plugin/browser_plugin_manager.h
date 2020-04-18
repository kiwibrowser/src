// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_BROWSER_PLUGIN_BROWSER_PLUGIN_MANAGER_H_
#define CONTENT_RENDERER_BROWSER_PLUGIN_BROWSER_PLUGIN_MANAGER_H_

#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_thread_observer.h"
#include "ipc/ipc_sender.h"

namespace content {

class BrowserPlugin;
class BrowserPluginDelegate;
class RenderFrame;

// BrowserPluginManager manages the routing of messages to the appropriate
// BrowserPlugin object based on its instance ID. There is one BrowserPlugin
// for the RenderThread.
class CONTENT_EXPORT BrowserPluginManager : public RenderThreadObserver {
 public:
  static BrowserPluginManager* Get();

  BrowserPluginManager();
  ~BrowserPluginManager() override;

  // Creates a new BrowserPlugin object.
  // BrowserPlugin is responsible for associating itself with the
  // BrowserPluginManager via AddBrowserPlugin. When it is destroyed, it is
  // responsible for removing its association via RemoveBrowserPlugin.
  // The |delegate| is expected to manage its own lifetime.
  // Generally BrowserPlugin calls DidDestroyElement() on the delegate and
  // right now the delegate destroys itself once it hears that callback.
  BrowserPlugin* CreateBrowserPlugin(
      RenderFrame* render_frame,
      const base::WeakPtr<BrowserPluginDelegate>& delegate);

  void Attach(int browser_plugin_instance_id);

  void Detach(int browser_plugin_instance_id);

  void AddBrowserPlugin(int browser_plugin_instance_id,
                        BrowserPlugin* browser_plugin);
  void RemoveBrowserPlugin(int browser_plugin_instance_id);
  BrowserPlugin* GetBrowserPlugin(int browser_plugin_instance_id) const;

  void UpdateFocusState();

  // Returns a new instance ID to be used by BrowserPlugin. Instance IDs are
  // unique per process.
  int GetNextInstanceID();

  bool Send(IPC::Message* msg);

  // RenderThreadObserver override.
  bool OnControlMessageReceived(const IPC::Message& message) override;

 private:
  // This map is keyed by guest instance IDs.
  base::IDMap<BrowserPlugin*> instances_;

  DISALLOW_COPY_AND_ASSIGN(BrowserPluginManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_BROWSER_PLUGIN_BROWSER_PLUGIN_MANAGER_H_
