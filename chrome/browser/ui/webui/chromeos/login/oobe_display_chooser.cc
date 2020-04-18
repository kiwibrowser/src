// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/oobe_display_chooser.h"

#include <stdint.h>

#include "ash/display/window_tree_host_manager.h"
#include "ash/shell.h"
#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/devices/input_device_manager.h"
#include "ui/events/devices/touchscreen_device.h"

using content::BrowserThread;

namespace chromeos {

namespace {

bool TouchSupportAvailable(const display::Display& display) {
  return display.touch_support() == display::Display::TouchSupport::AVAILABLE;
}

// TODO(felixe): More context at crbug.com/738885
const uint16_t kDeviceIds[] = {0x0457, 0x266e};

// Returns true if |vendor_id| is a valid vendor id that may be made the primary
// display.
bool IsWhiteListedVendorId(uint16_t vendor_id) {
  return base::ContainsValue(kDeviceIds, vendor_id);
}

}  // namespace

OobeDisplayChooser::OobeDisplayChooser()
    : scoped_observer_(this), weak_ptr_factory_(this) {}

OobeDisplayChooser::~OobeDisplayChooser() {}

void OobeDisplayChooser::TryToPlaceUiOnTouchDisplay() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Don't (potentially) queue a second task to run MoveToTouchDisplay if one
  // already is queued.
  if (weak_ptr_factory_.HasWeakPtrs())
    return;

  display::Display primary_display =
      display::Screen::GetScreen()->GetPrimaryDisplay();

  if (primary_display.is_valid() && !TouchSupportAvailable(primary_display)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&OobeDisplayChooser::MaybeMoveToTouchDisplay,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void OobeDisplayChooser::MaybeMoveToTouchDisplay() {
  ui::InputDeviceManager* input_device_manager =
      ui::InputDeviceManager::GetInstance();
  if (input_device_manager->AreDeviceListsComplete() &&
      input_device_manager->AreTouchscreenTargetDisplaysValid()) {
    MoveToTouchDisplay();
  } else if (!scoped_observer_.IsObserving(input_device_manager)) {
    scoped_observer_.Add(input_device_manager);
  }
}

void OobeDisplayChooser::MoveToTouchDisplay() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  scoped_observer_.RemoveAll();

  const ui::InputDeviceManager* input_device_manager =
      ui::InputDeviceManager::GetInstance();
  for (const ui::TouchscreenDevice& device :
       input_device_manager->GetTouchscreenDevices()) {
    if (IsWhiteListedVendorId(device.vendor_id) &&
        device.target_display_id != display::kInvalidDisplayId) {
      ash::Shell::Get()->window_tree_host_manager()->SetPrimaryDisplayId(
          device.target_display_id);
      break;
    }
  }
}

void OobeDisplayChooser::OnTouchDeviceAssociationChanged() {
  MaybeMoveToTouchDisplay();
}

void OobeDisplayChooser::OnTouchscreenDeviceConfigurationChanged() {
  MaybeMoveToTouchDisplay();
}

void OobeDisplayChooser::OnDeviceListsComplete() {
  MaybeMoveToTouchDisplay();
}

}  // namespace chromeos
