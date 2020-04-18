// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/browser_devtools_agent_host.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "content/browser/devtools/devtools_session.h"
#include "content/browser/devtools/protocol/browser_handler.h"
#include "content/browser/devtools/protocol/io_handler.h"
#include "content/browser/devtools/protocol/memory_handler.h"
#include "content/browser/devtools/protocol/protocol.h"
#include "content/browser/devtools/protocol/security_handler.h"
#include "content/browser/devtools/protocol/system_info_handler.h"
#include "content/browser/devtools/protocol/target_handler.h"
#include "content/browser/devtools/protocol/tethering_handler.h"
#include "content/browser/devtools/protocol/tracing_handler.h"
#include "content/browser/frame_host/frame_tree_node.h"

namespace content {

scoped_refptr<DevToolsAgentHost> DevToolsAgentHost::CreateForBrowser(
    scoped_refptr<base::SingleThreadTaskRunner> tethering_task_runner,
    const CreateServerSocketCallback& socket_callback) {
  return new BrowserDevToolsAgentHost(
      tethering_task_runner, socket_callback, false);
}

scoped_refptr<DevToolsAgentHost> DevToolsAgentHost::CreateForDiscovery() {
  CreateServerSocketCallback null_callback;
  return new BrowserDevToolsAgentHost(nullptr, std::move(null_callback), true);
}

BrowserDevToolsAgentHost::BrowserDevToolsAgentHost(
    scoped_refptr<base::SingleThreadTaskRunner> tethering_task_runner,
    const CreateServerSocketCallback& socket_callback,
    bool only_discovery)
    : DevToolsAgentHostImpl(base::GenerateGUID()),
      tethering_task_runner_(tethering_task_runner),
      socket_callback_(socket_callback),
      only_discovery_(only_discovery) {
  NotifyCreated();
}

BrowserDevToolsAgentHost::~BrowserDevToolsAgentHost() {
}

bool BrowserDevToolsAgentHost::AttachSession(DevToolsSession* session) {
  if (session->restricted())
    return false;

  session->SetBrowserOnly(true);
  session->AddHandler(
      base::WrapUnique(new protocol::TargetHandler(true /* browser_only */)));
  if (only_discovery_)
    return true;

  session->AddHandler(base::WrapUnique(new protocol::BrowserHandler()));
  session->AddHandler(base::WrapUnique(new protocol::IOHandler(
      GetIOContext())));
  session->AddHandler(base::WrapUnique(new protocol::MemoryHandler()));
  session->AddHandler(base::WrapUnique(new protocol::SecurityHandler()));
  session->AddHandler(base::WrapUnique(new protocol::SystemInfoHandler()));
  session->AddHandler(base::WrapUnique(new protocol::TetheringHandler(
      socket_callback_, tethering_task_runner_)));
  session->AddHandler(
      base::WrapUnique(new protocol::TracingHandler(nullptr, GetIOContext())));
  return true;
}

void BrowserDevToolsAgentHost::DetachSession(DevToolsSession* session) {}

std::string BrowserDevToolsAgentHost::GetType() {
  return kTypeBrowser;
}

std::string BrowserDevToolsAgentHost::GetTitle() {
  return "";
}

GURL BrowserDevToolsAgentHost::GetURL() {
  return GURL();
}

bool BrowserDevToolsAgentHost::Activate() {
  return false;
}

bool BrowserDevToolsAgentHost::Close() {
  return false;
}

void BrowserDevToolsAgentHost::Reload() {
}

void BrowserDevToolsAgentHost::DispatchProtocolMessage(
    DevToolsSession* session,
    const std::string& message) {
  session->DispatchProtocolMessage(message);
}

}  // content
