// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/content/screen_orientation_delegate_chromeos.h"

#include "ash/display/screen_orientation_controller.h"
#include "ash/shell.h"
#include "content/public/browser/web_contents.h"

namespace ash {
namespace {

OrientationLockType ToAshOrientationLockType(
    blink::WebScreenOrientationLockType blink_orientation_lock) {
  switch (blink_orientation_lock) {
    case blink::kWebScreenOrientationLockDefault:
    case blink::kWebScreenOrientationLockAny:
      return OrientationLockType::kAny;
    case blink::kWebScreenOrientationLockPortrait:
      return OrientationLockType::kPortrait;
    case blink::kWebScreenOrientationLockPortraitPrimary:
      return OrientationLockType::kPortraitPrimary;
    case blink::kWebScreenOrientationLockPortraitSecondary:
      return OrientationLockType::kPortraitSecondary;
    case blink::kWebScreenOrientationLockLandscape:
      return OrientationLockType::kLandscape;
    case blink::kWebScreenOrientationLockLandscapePrimary:
      return OrientationLockType::kLandscapePrimary;
    case blink::kWebScreenOrientationLockLandscapeSecondary:
      return OrientationLockType::kLandscapeSecondary;
    case blink::kWebScreenOrientationLockNatural:
      return OrientationLockType::kNatural;
  }
  return OrientationLockType::kAny;
}

}  // namespace

ScreenOrientationDelegateChromeos::ScreenOrientationDelegateChromeos() {
  content::WebContents::SetScreenOrientationDelegate(this);
}

ScreenOrientationDelegateChromeos::~ScreenOrientationDelegateChromeos() {
  content::WebContents::SetScreenOrientationDelegate(nullptr);
}

bool ScreenOrientationDelegateChromeos::FullScreenRequired(
    content::WebContents* web_contents) {
  return true;
}

void ScreenOrientationDelegateChromeos::Lock(
    content::WebContents* web_contents,
    blink::WebScreenOrientationLockType orientation_lock) {
  Shell::Get()->screen_orientation_controller()->LockOrientationForWindow(
      web_contents->GetNativeView(),
      ToAshOrientationLockType(orientation_lock));
}

bool ScreenOrientationDelegateChromeos::ScreenOrientationProviderSupported() {
  return Shell::Get()
      ->screen_orientation_controller()
      ->ScreenOrientationProviderSupported();
}

void ScreenOrientationDelegateChromeos::Unlock(
    content::WebContents* web_contents) {
  Shell::Get()->screen_orientation_controller()->UnlockOrientationForWindow(
      web_contents->GetNativeView());
}

}  // namespace ash
