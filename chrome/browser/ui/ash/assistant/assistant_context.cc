// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/assistant/assistant_context.h"

#include "chrome/browser/ui/ash/assistant/assistant_context_util.h"

AssistantContext::AssistantContext() = default;
AssistantContext::~AssistantContext() = default;

void AssistantContext::RequestAssistantStructure(
    RequestAssistantStructureCallback callback) {
  RequestAssistantStructureForActiveBrowserWindow(std::move(callback));
}
