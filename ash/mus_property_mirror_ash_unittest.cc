// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/mus_property_mirror_ash.h"

#include <string>

#include "ash/public/cpp/shelf_types.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/test/ash_test_base.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"

namespace ash {

using MusPropertyMirrorAshTest = AshTestBase;

// Ensure the property mirror can copy primitive properties between windows.
TEST_F(MusPropertyMirrorAshTest, PrimitiveProperties) {
  MusPropertyMirrorAsh mus_property_mirror_ash;
  std::unique_ptr<aura::Window> window_1(CreateTestWindow());
  std::unique_ptr<aura::Window> window_2(CreateTestWindow());

  EXPECT_EQ(TYPE_UNDEFINED, window_1->GetProperty(kShelfItemTypeKey));
  EXPECT_EQ(TYPE_UNDEFINED, window_2->GetProperty(kShelfItemTypeKey));
  window_1->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_APP));
  EXPECT_EQ(TYPE_APP, window_1->GetProperty(kShelfItemTypeKey));
  mus_property_mirror_ash.MirrorPropertyFromWidgetWindowToRootWindow(
      window_1.get(), window_2.get(), kShelfItemTypeKey);
  EXPECT_EQ(TYPE_APP, window_2->GetProperty(kShelfItemTypeKey));

  EXPECT_FALSE(window_1->GetProperty(aura::client::kDrawAttentionKey));
  EXPECT_FALSE(window_2->GetProperty(aura::client::kDrawAttentionKey));
  window_1->SetProperty(aura::client::kDrawAttentionKey, true);
  EXPECT_TRUE(window_1->GetProperty(aura::client::kDrawAttentionKey));
  mus_property_mirror_ash.MirrorPropertyFromWidgetWindowToRootWindow(
      window_1.get(), window_2.get(), aura::client::kDrawAttentionKey);
  EXPECT_TRUE(window_2->GetProperty(aura::client::kDrawAttentionKey));
}

// Ensure the property mirror can copy owned object properties between windows.
TEST_F(MusPropertyMirrorAshTest, OwnedProperties) {
  MusPropertyMirrorAsh mus_property_mirror_ash;
  std::unique_ptr<aura::Window> window_1(CreateTestWindow());
  std::unique_ptr<aura::Window> window_2(CreateTestWindow());

  EXPECT_EQ(nullptr, window_1->GetProperty(aura::client::kTitleKey));
  EXPECT_EQ(nullptr, window_2->GetProperty(aura::client::kTitleKey));
  window_1->SetProperty(aura::client::kTitleKey,
                        new base::string16(base::ASCIIToUTF16("abc")));
  EXPECT_EQ(base::ASCIIToUTF16("abc"),
            *window_1->GetProperty(aura::client::kTitleKey));
  mus_property_mirror_ash.MirrorPropertyFromWidgetWindowToRootWindow(
      window_1.get(), window_2.get(), aura::client::kTitleKey);
  EXPECT_EQ(base::ASCIIToUTF16("abc"),
            *window_2->GetProperty(aura::client::kTitleKey));
  EXPECT_NE(window_1->GetProperty(aura::client::kTitleKey),
            window_2->GetProperty(aura::client::kTitleKey));

  window_1->ClearProperty(aura::client::kWindowIconKey);
  EXPECT_EQ(nullptr, window_1->GetProperty(aura::client::kWindowIconKey));
  window_2->ClearProperty(aura::client::kWindowIconKey);
  EXPECT_EQ(nullptr, window_2->GetProperty(aura::client::kWindowIconKey));
  window_1->SetProperty(aura::client::kWindowIconKey, new gfx::ImageSkia());
  EXPECT_NE(nullptr, window_1->GetProperty(aura::client::kWindowIconKey));
  mus_property_mirror_ash.MirrorPropertyFromWidgetWindowToRootWindow(
      window_1.get(), window_2.get(), aura::client::kWindowIconKey);
  EXPECT_NE(nullptr, window_2->GetProperty(aura::client::kWindowIconKey));
  EXPECT_NE(window_1->GetProperty(aura::client::kWindowIconKey),
            window_2->GetProperty(aura::client::kWindowIconKey));
}

}  // namespace ash
