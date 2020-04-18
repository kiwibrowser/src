// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/toolbar/component_toolbar_actions_factory.h"

#include "base/macros.h"
#include "chrome/browser/extensions/extension_action_test_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/value_builder.h"

class ComponentToolbarActionsFactoryTest
    : public extensions::ExtensionServiceTestBase {
 public:
  ComponentToolbarActionsFactoryTest() {}
  ~ComponentToolbarActionsFactoryTest() override {}

  void SetUp() override {
    extensions::ExtensionServiceTestBase::SetUp();
    InitializeEmptyExtensionService();
    actions_factory_ =
        extensions::extension_action_test_util::
            CreateToolbarModelForProfileWithoutWaitingForReady(profile())
                ->component_actions_factory();
  }

 protected:
  ComponentToolbarActionsFactory* actions_factory_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(ComponentToolbarActionsFactoryTest);
};

TEST_F(ComponentToolbarActionsFactoryTest, GetInitialIds) {
  std::string id1("id1");
  std::string id2("id2");
  std::string id3("id3");

  actions_factory_->OnAddComponentActionBeforeInit(id1);
  actions_factory_->OnAddComponentActionBeforeInit(id2);
  actions_factory_->OnAddComponentActionBeforeInit(id3);
  actions_factory_->OnRemoveComponentActionBeforeInit(id2);

  std::set<std::string> initial_ids =
      actions_factory_->GetInitialComponentIds();
  EXPECT_TRUE(base::ContainsKey(initial_ids, id1));
  EXPECT_FALSE(base::ContainsKey(initial_ids, id2));
  EXPECT_TRUE(base::ContainsKey(initial_ids, id3));
}
