// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include "chrome/browser/ui/cocoa/find_bar/find_bar_bridge.h"

#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_cocoa_controller.h"
#include "ui/gfx/range/range.h"

// static
bool FindBarBridge::disable_animations_during_testing_ = false;

FindBarBridge::FindBarBridge(Browser* browser)
    : find_bar_controller_(NULL), audible_alerts_(0) {
  cocoa_controller_ = [[FindBarCocoaController alloc] initWithBrowser:browser];
  [cocoa_controller_ setFindBarBridge:this];
}

FindBarBridge::~FindBarBridge() {
  [cocoa_controller_ release];
}

void FindBarBridge::SetFindBarController(
    FindBarController* find_bar_controller) {
  find_bar_controller_ = find_bar_controller;
}

FindBarController* FindBarBridge::GetFindBarController() const {
  return find_bar_controller_;
}

FindBarTesting* FindBarBridge::GetFindBarTesting() {
  return this;
}

void FindBarBridge::Show(bool animate) {
  bool really_animate = animate && !disable_animations_during_testing_;
  [cocoa_controller_ showFindBar:(really_animate ? YES : NO)];
}

void FindBarBridge::Hide(bool animate) {
  bool really_animate = animate && !disable_animations_during_testing_;
  [cocoa_controller_ hideFindBar:(really_animate ? YES : NO)];
}

void FindBarBridge::SetFocusAndSelection() {
  [cocoa_controller_ setFocusAndSelection];
}

void FindBarBridge::ClearResults(const FindNotificationDetails& results) {
  [cocoa_controller_ clearResults:results];
}

void FindBarBridge::SetFindTextAndSelectedRange(
    const base::string16& find_text,
    const gfx::Range& selected_range) {
  [cocoa_controller_ setFindText:base::SysUTF16ToNSString(find_text)
                   selectedRange:selected_range.ToNSRange()];
}

base::string16 FindBarBridge::GetFindText() {
  return base::SysNSStringToUTF16([cocoa_controller_ findText]);
}

gfx::Range FindBarBridge::GetSelectedRange() {
  return gfx::Range([cocoa_controller_ selectedRange]);
}

void FindBarBridge::UpdateUIForFindResult(const FindNotificationDetails& result,
                                          const base::string16& find_text) {
  [cocoa_controller_ updateUIForFindResult:result withText:find_text];
}

void FindBarBridge::AudibleAlert() {
  // Beep beep, beep beep, Yeah!
  ++audible_alerts_;
  NSBeep();
}

bool FindBarBridge::IsFindBarVisible() {
  return [cocoa_controller_ isFindBarVisible] ? true : false;
}

void FindBarBridge::MoveWindowIfNecessary(const gfx::Rect& selection_rect) {
  // See FindBarCocoaController moveFindBarToAvoidRect.
}

void FindBarBridge::StopAnimation() {
  [cocoa_controller_ stopAnimation];
}

void FindBarBridge::RestoreSavedFocus() {
  [cocoa_controller_ restoreSavedFocus];
}

bool FindBarBridge::HasGlobalFindPasteboard() {
  return true;
}

void FindBarBridge::UpdateFindBarForChangedWebContents() {
  [cocoa_controller_ updateFindBarForChangedWebContents];
}

bool FindBarBridge::GetFindBarWindowInfo(gfx::Point* position,
                                         bool* fully_visible) {
  NSWindow* window = [[cocoa_controller_ view] window];
  bool window_visible = [window isVisible] ? true : false;
  if (position) {
    if (window_visible)
      *position = [cocoa_controller_ findBarWindowPosition];
    else
      *position = gfx::Point(0, 0);
  }
  if (fully_visible) {
    *fully_visible = window_visible &&
        [cocoa_controller_ isFindBarVisible] &&
        ![cocoa_controller_ isFindBarAnimating];
  }
  return window_visible;
}

base::string16 FindBarBridge::GetFindSelectedText() {
  // This function is currently only used in Views.
  NOTIMPLEMENTED();
  return base::string16();
}

base::string16 FindBarBridge::GetMatchCountText() {
  return base::SysNSStringToUTF16([cocoa_controller_ matchCountText]);
}

int FindBarBridge::GetWidth() {
  return [cocoa_controller_ findBarWidth];
}

size_t FindBarBridge::GetAudibleAlertCount() {
  return audible_alerts_;
}
