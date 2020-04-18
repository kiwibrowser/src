// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/accelerators_cocoa.h"

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "chrome/app/chrome_command_ids.h"
#include "printing/buildflags/buildflags.h"
#import "ui/base/accelerators/platform_accelerator_cocoa.h"
#import "ui/events/cocoa/cocoa_event_utils.h"
#import "ui/events/keycodes/keyboard_code_conversion_mac.h"

namespace {

// These accelerators are not associated with a command_id.
const struct AcceleratorListing {
  NSUInteger modifiers;  // The Cocoa modifiers.
  ui::KeyboardCode key_code;  // The key used for cross-platform compatibility.
} kAcceleratorList [] = {
  {NSCommandKeyMask | NSAlternateKeyMask, ui::VKEY_H},
  {NSCommandKeyMask | NSAlternateKeyMask, ui::VKEY_W},
  {NSCommandKeyMask | NSAlternateKeyMask | NSShiftKeyMask, ui::VKEY_V},
  {NSCommandKeyMask, ui::VKEY_E},
  {NSCommandKeyMask, ui::VKEY_J},
  {NSCommandKeyMask, ui::VKEY_OEM_1},
  {NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_OEM_1},
  {NSCommandKeyMask, ui::VKEY_OEM_COMMA},
  {NSCommandKeyMask | NSControlKeyMask, ui::VKEY_SPACE},
};

const struct AcceleratorMapping {
  int command_id;
  NSUInteger modifiers;  // The Cocoa modifiers.
  ui::KeyboardCode key_code;  // The key used for cross-platform compatibility.
} kAcceleratorMap[] = {
  // Accelerators used in the toolbar menu.
  {IDC_CLEAR_BROWSING_DATA, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_BACK},
  {IDC_COPY, NSCommandKeyMask, ui::VKEY_C},
  {IDC_CUT, NSCommandKeyMask, ui::VKEY_X},
  {IDC_DEV_TOOLS, NSCommandKeyMask | NSAlternateKeyMask, ui::VKEY_I},
  {IDC_DEV_TOOLS_CONSOLE, NSCommandKeyMask | NSAlternateKeyMask, ui::VKEY_J},
  {IDC_FIND, NSCommandKeyMask, ui::VKEY_F},
  {IDC_FULLSCREEN, NSCommandKeyMask | NSControlKeyMask, ui::VKEY_F},
  {IDC_NEW_INCOGNITO_WINDOW, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_N},
  {IDC_NEW_TAB, NSCommandKeyMask, ui::VKEY_T},
  {IDC_NEW_WINDOW, NSCommandKeyMask, ui::VKEY_N},
  {IDC_PASTE, NSCommandKeyMask, ui::VKEY_V},
  {IDC_PRINT, NSCommandKeyMask, ui::VKEY_P},
  {IDC_RESTORE_TAB, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_T},
  {IDC_SAVE_PAGE, NSCommandKeyMask, ui::VKEY_S},
  {IDC_SHOW_BOOKMARK_BAR, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_B},
  {IDC_SHOW_BOOKMARK_MANAGER, NSCommandKeyMask | NSAlternateKeyMask,
   ui::VKEY_B},
  {IDC_BOOKMARK_PAGE, NSCommandKeyMask, ui::VKEY_D},
  {IDC_SHOW_DOWNLOADS, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_J},
  {IDC_SHOW_HISTORY, NSCommandKeyMask, ui::VKEY_Y},
  {IDC_VIEW_SOURCE, NSCommandKeyMask | NSAlternateKeyMask, ui::VKEY_U},
  {IDC_ZOOM_MINUS, NSCommandKeyMask, ui::VKEY_OEM_MINUS},
  {IDC_ZOOM_PLUS, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_OEM_PLUS},

  // Accelerators used in MainMenu.xib, but not the toolbar menu.
  {IDC_HIDE_APP, NSCommandKeyMask, ui::VKEY_H},
  {IDC_EXIT, NSCommandKeyMask, ui::VKEY_Q},
  {IDC_OPEN_FILE, NSCommandKeyMask, ui::VKEY_O},
  {IDC_FOCUS_LOCATION, NSCommandKeyMask, ui::VKEY_L},
  {IDC_CLOSE_WINDOW, NSCommandKeyMask, ui::VKEY_W},
  {IDC_EMAIL_PAGE_LOCATION, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_I},
#if BUILDFLAG(ENABLE_PRINTING)
  {IDC_BASIC_PRINT, NSCommandKeyMask | NSAlternateKeyMask, ui::VKEY_P},
#endif  // ENABLE_PRINTING
  {IDC_CONTENT_CONTEXT_UNDO, NSCommandKeyMask, ui::VKEY_Z},
  {IDC_CONTENT_CONTEXT_REDO, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_Z},
  {IDC_CONTENT_CONTEXT_CUT, NSCommandKeyMask, ui::VKEY_X},
  {IDC_CONTENT_CONTEXT_COPY, NSCommandKeyMask, ui::VKEY_C},
  {IDC_CONTENT_CONTEXT_PASTE, NSCommandKeyMask, ui::VKEY_V},
  {IDC_CONTENT_CONTEXT_PASTE_AND_MATCH_STYLE,
   NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_V},
  {IDC_CONTENT_CONTEXT_SELECTALL, NSCommandKeyMask, ui::VKEY_A},
  {IDC_FOCUS_SEARCH, NSCommandKeyMask | NSAlternateKeyMask, ui::VKEY_F},
  {IDC_FIND_NEXT, NSCommandKeyMask, ui::VKEY_G},
  {IDC_FIND_PREVIOUS, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_G},
  {IDC_ZOOM_PLUS, NSCommandKeyMask, ui::VKEY_OEM_PLUS},
  {IDC_ZOOM_MINUS, NSCommandKeyMask, ui::VKEY_OEM_MINUS},
  {IDC_STOP, NSCommandKeyMask, ui::VKEY_OEM_PERIOD},
  {IDC_RELOAD, NSCommandKeyMask, ui::VKEY_R},
  {IDC_RELOAD_BYPASSING_CACHE, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_R},
  {IDC_ZOOM_NORMAL, NSCommandKeyMask, ui::VKEY_0},
  {IDC_HOME, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_H},
  {IDC_BACK, NSCommandKeyMask, ui::VKEY_OEM_4},
  {IDC_FORWARD, NSCommandKeyMask, ui::VKEY_OEM_6},
  {IDC_BOOKMARK_ALL_TABS, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_D},
  {IDC_MINIMIZE_WINDOW, NSCommandKeyMask, ui::VKEY_M},
  {IDC_SELECT_NEXT_TAB, NSCommandKeyMask | NSAlternateKeyMask, ui::VKEY_RIGHT},
  {IDC_SELECT_PREVIOUS_TAB, NSCommandKeyMask | NSAlternateKeyMask,
   ui::VKEY_LEFT},
  {IDC_HELP_PAGE_VIA_MENU, NSCommandKeyMask | NSShiftKeyMask, ui::VKEY_OEM_2},
};

// Create a Cocoa platform accelerator given a cross platform |key_code| and
// the |cocoa_modifiers|.
std::unique_ptr<ui::PlatformAccelerator> PlatformAcceleratorFromKeyCode(
    ui::KeyboardCode key_code,
    NSUInteger cocoa_modifiers) {
  unichar shifted_character;
  int result = ui::MacKeyCodeForWindowsKeyCode(key_code, cocoa_modifiers,
                                               &shifted_character, nullptr);
  DCHECK(result != -1);
  NSString* key_equivalent =
      [NSString stringWithFormat:@"%C", shifted_character];

  return std::unique_ptr<ui::PlatformAccelerator>(
      new ui::PlatformAcceleratorCocoa(key_equivalent, cocoa_modifiers));
}

// Create a cross platform accelerator given a cross platform |key_code| and
// the |cocoa_modifiers|.
ui::Accelerator AcceleratorFromKeyCode(ui::KeyboardCode key_code,
                                       NSUInteger cocoa_modifiers) {
  int cross_platform_modifiers = ui::EventFlagsFromModifiers(cocoa_modifiers);
  ui::Accelerator accelerator(key_code, cross_platform_modifiers);

  std::unique_ptr<ui::PlatformAccelerator> platform_accelerator =
      PlatformAcceleratorFromKeyCode(key_code, cocoa_modifiers);
  accelerator.set_platform_accelerator(std::move(platform_accelerator));
  return accelerator;
}

}  // namespace

AcceleratorsCocoa::AcceleratorsCocoa() {
  for (size_t i = 0; i < arraysize(kAcceleratorMap); ++i) {
    const AcceleratorMapping& entry = kAcceleratorMap[i];
    ui::Accelerator accelerator =
        AcceleratorFromKeyCode(entry.key_code, entry.modifiers);
    accelerators_.insert(std::make_pair(entry.command_id, accelerator));
  }

  for (size_t i = 0; i < arraysize(kAcceleratorList); ++i) {
    const AcceleratorListing& entry = kAcceleratorList[i];
    ui::Accelerator accelerator =
        AcceleratorFromKeyCode(entry.key_code, entry.modifiers);
    accelerator_vector_.push_back(accelerator);
  }
}

AcceleratorsCocoa::~AcceleratorsCocoa() {}

// static
AcceleratorsCocoa* AcceleratorsCocoa::GetInstance() {
  return base::Singleton<AcceleratorsCocoa>::get();
}

const ui::Accelerator* AcceleratorsCocoa::GetAcceleratorForCommand(
    int command_id) {
  AcceleratorMap::iterator it = accelerators_.find(command_id);
  if (it == accelerators_.end())
    return NULL;
  return &it->second;
}

const ui::Accelerator* AcceleratorsCocoa::GetAcceleratorForHotKey(
    NSString* key_equivalent, NSUInteger modifiers) const {
  for (AcceleratorVector::const_iterator it = accelerator_vector_.begin();
       it != accelerator_vector_.end();
       ++it) {
    const ui::Accelerator& accelerator = *it;
    const ui::PlatformAcceleratorCocoa* platform_accelerator =
        static_cast<const ui::PlatformAcceleratorCocoa*>(
            accelerator.platform_accelerator());
    unichar shifted_character;
    int result = ui::MacKeyCodeForWindowsKeyCode(
        accelerator.key_code(), platform_accelerator->modifier_mask(),
        &shifted_character, nullptr);
    if (result == -1)
      return NULL;

    // Check for a match in the modifiers and key_equivalent.
    NSUInteger mask = platform_accelerator->modifier_mask();
    BOOL maskEqual =
        (mask == modifiers) || ((mask & (~NSShiftKeyMask)) == modifiers);
    NSString* string = [NSString stringWithFormat:@"%C", shifted_character];
    if ([string isEqual:key_equivalent] && maskEqual)
      return &*it;
  }

  return NULL;
}
