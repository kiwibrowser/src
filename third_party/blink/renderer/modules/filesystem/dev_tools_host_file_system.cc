// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/filesystem/dev_tools_host_file_system.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/dev_tools_host.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/modules/filesystem/dom_file_system.h"
#include "third_party/blink/renderer/platform/json/json_values.h"

namespace blink {

DOMFileSystem* DevToolsHostFileSystem::isolatedFileSystem(
    DevToolsHost& host,
    const String& file_system_name,
    const String& root_url) {
  ExecutionContext* context = host.FrontendFrame()->GetDocument();
  return DOMFileSystem::Create(context, file_system_name,
                               kFileSystemTypeIsolated, KURL(root_url));
}

void DevToolsHostFileSystem::upgradeDraggedFileSystemPermissions(
    DevToolsHost& host,
    DOMFileSystem* dom_file_system) {
  std::unique_ptr<JSONObject> message = JSONObject::Create();
  message->SetInteger("id", 0);
  message->SetString("method", "upgradeDraggedFileSystemPermissions");
  std::unique_ptr<JSONArray> params = JSONArray::Create();
  params->PushString(dom_file_system->RootURL().GetString());
  message->SetArray("params", std::move(params));
  host.sendMessageToEmbedder(message->ToJSONString());
}

}  // namespace blink
