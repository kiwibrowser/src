// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CONTEXT_H_
#define CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CONTEXT_H_

#include "base/macros.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"

// Class for Assistant to retrieve view hierarchy information from browser
// windows.
class AssistantContext : public chromeos::assistant::mojom::Context {
 public:
  AssistantContext();
  ~AssistantContext() override;

  // mojom::Context overrides
  void RequestAssistantStructure(
      RequestAssistantStructureCallback callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AssistantContext);
};

#endif  // CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CONTEXT_H_
