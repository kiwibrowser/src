// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_MOCK_COMPONENT_TOOLBAR_ACTIONS_FACTORY_H_
#define CHROME_BROWSER_UI_TOOLBAR_MOCK_COMPONENT_TOOLBAR_ACTIONS_FACTORY_H_

#include "base/macros.h"
#include "chrome/browser/ui/toolbar/component_toolbar_actions_factory.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"

class Browser;

// Mock class used to substitute ComponentToolbarActionsFactory in tests.
class MockComponentToolbarActionsFactory
    : public ComponentToolbarActionsFactory {
 public:
  static const char kActionIdForTesting[];

  explicit MockComponentToolbarActionsFactory(Profile* profile);
  ~MockComponentToolbarActionsFactory() override;

  // ComponentToolbarActionsFactory:
  std::set<std::string> GetInitialComponentIds() override;
  std::unique_ptr<ToolbarActionViewController> GetComponentToolbarActionForId(
      const std::string& id,
      Browser* browser,
      ToolbarActionsBar* bar) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockComponentToolbarActionsFactory);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_MOCK_COMPONENT_TOOLBAR_ACTIONS_FACTORY_H_
