// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/browser_plugin/browser_plugin_manager.h"

#include <memory>

#include "content/common/browser_plugin/browser_plugin_constants.h"
#include "content/common/browser_plugin/browser_plugin_messages.h"
#include "content/common/frame_messages.h"
#include "content/public/renderer/browser_plugin_delegate.h"
#include "content/renderer/browser_plugin/browser_plugin.h"
#include "content/renderer/render_thread_impl.h"
#include "ipc/ipc_message_macros.h"

namespace content {

// static
BrowserPluginManager* BrowserPluginManager::Get() {
  if (!RenderThreadImpl::current())
    return nullptr;
  return RenderThreadImpl::current()->browser_plugin_manager();
}

BrowserPluginManager::BrowserPluginManager() {
}

BrowserPluginManager::~BrowserPluginManager() {
}

void BrowserPluginManager::AddBrowserPlugin(
    int browser_plugin_instance_id,
    BrowserPlugin* browser_plugin) {
  instances_.AddWithID(browser_plugin, browser_plugin_instance_id);
}

void BrowserPluginManager::RemoveBrowserPlugin(int browser_plugin_instance_id) {
  instances_.Remove(browser_plugin_instance_id);
}

BrowserPlugin* BrowserPluginManager::GetBrowserPlugin(
    int browser_plugin_instance_id) const {
  return instances_.Lookup(browser_plugin_instance_id);
}

int BrowserPluginManager::GetNextInstanceID() {
  return RenderThreadImpl::current()->GenerateRoutingID();
}

void BrowserPluginManager::UpdateFocusState() {
  base::IDMap<BrowserPlugin*>::iterator iter(&instances_);
  while (!iter.IsAtEnd()) {
    iter.GetCurrentValue()->UpdateGuestFocusState(blink::kWebFocusTypeNone);
    iter.Advance();
  }
}

void BrowserPluginManager::Attach(int browser_plugin_instance_id) {
  BrowserPlugin* plugin = GetBrowserPlugin(browser_plugin_instance_id);
  if (plugin)
    plugin->Attach();
}

void BrowserPluginManager::Detach(int browser_plugin_instance_id) {
  BrowserPlugin* plugin = GetBrowserPlugin(browser_plugin_instance_id);
  if (plugin)
    plugin->Detach();
}

BrowserPlugin* BrowserPluginManager::CreateBrowserPlugin(
    RenderFrame* render_frame,
    const base::WeakPtr<BrowserPluginDelegate>& delegate) {
  return new BrowserPlugin(render_frame, delegate);
}

bool BrowserPluginManager::OnControlMessageReceived(
    const IPC::Message& message) {
  if (!BrowserPlugin::ShouldForwardToBrowserPlugin(message))
    return false;

  int browser_plugin_instance_id = browser_plugin::kInstanceIDNone;
  // All allowed messages must have |browser_plugin_instance_id| as their
  // first parameter.
  base::PickleIterator iter(message);
  bool success = iter.ReadInt(&browser_plugin_instance_id);
  DCHECK(success);
  BrowserPlugin* plugin = GetBrowserPlugin(browser_plugin_instance_id);
  return plugin && plugin->OnMessageReceived(message);
}

bool BrowserPluginManager::Send(IPC::Message* msg) {
  return RenderThreadImpl::current()->Send(msg);
}

}  // namespace content
