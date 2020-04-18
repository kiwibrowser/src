// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/toolbar/mock_component_toolbar_actions_factory.h"

#include <memory>

#include "chrome/browser/ui/toolbar/test_toolbar_action_view_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"

// static
const char MockComponentToolbarActionsFactory::kActionIdForTesting[] =
    "mock_action";

MockComponentToolbarActionsFactory::MockComponentToolbarActionsFactory(
    Profile* profile)
    : ComponentToolbarActionsFactory(profile) {}

MockComponentToolbarActionsFactory::~MockComponentToolbarActionsFactory() {}

std::set<std::string>
MockComponentToolbarActionsFactory::GetInitialComponentIds() {
  return {kActionIdForTesting};
}

std::unique_ptr<ToolbarActionViewController>
MockComponentToolbarActionsFactory::GetComponentToolbarActionForId(
    const std::string& id,
    Browser* browser,
    ToolbarActionsBar* bar) {
  DCHECK_EQ(kActionIdForTesting, id);
  return std::make_unique<TestToolbarActionViewController>(
      MockComponentToolbarActionsFactory::kActionIdForTesting);
}
