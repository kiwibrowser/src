// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/dev_mode_bubble_delegate.h"

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/feature_switch.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/ui_base_features.h"

namespace extensions {

class DevModeBubbleDelegateMdUiUnitTest
    : public ExtensionServiceTestBase,
      public testing::WithParamInterface<bool> {
 public:
  DevModeBubbleDelegateMdUiUnitTest() = default;
  ~DevModeBubbleDelegateMdUiUnitTest() override = default;

  void SetUp() override {
    ExtensionServiceTestBase::SetUp();

    bool secondary_md_ui_enabled = GetParam();
    if (secondary_md_ui_enabled)
      scoped_feature_list_.InitAndEnableFeature(features::kSecondaryUiMd);
    else
      scoped_feature_list_.InitAndDisableFeature(features::kSecondaryUiMd);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(DevModeBubbleDelegateMdUiUnitTest);
};

TEST_P(DevModeBubbleDelegateMdUiUnitTest, ExtensionServiceTestBase) {
  FeatureSwitch::ScopedOverride dev_mode_highlighting(
      FeatureSwitch::force_dev_mode_highlighting(), true);

  InitializeEmptyExtensionService();

  scoped_refptr<const Extension> extension = ExtensionBuilder("test").Build();
  service()->AddExtension(extension.get());

  DevModeBubbleDelegate bubble_delegate(profile());
  EXPECT_TRUE(bubble_delegate.ShouldIncludeExtension(extension.get()));

  // The Cocoa version of the bubble doesn't have a dismiss 'x', so needs a
  // dedicated 'cancel' button.
  bool should_have_cancel_button =
#if defined(OS_MACOSX)
      !ui::MaterialDesignController::IsSecondaryUiMaterial();
#else
      false;
#endif

  EXPECT_EQ(should_have_cancel_button,
            !bubble_delegate.GetDismissButtonLabel().empty());
}

INSTANTIATE_TEST_CASE_P(, DevModeBubbleDelegateMdUiUnitTest, testing::Bool());

}  // namespace extensions
