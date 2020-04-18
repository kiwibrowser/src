// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_OOBE_DISPLAY_CHOOSER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_OOBE_DISPLAY_CHOOSER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "ui/events/devices/input_device_event_observer.h"

namespace ui {
class InputDeviceManager;
}

namespace chromeos {

class OobeDisplayChooser : public ui::InputDeviceEventObserver {
 public:
  OobeDisplayChooser();
  ~OobeDisplayChooser() override;

  // Tries to put the OOBE UI on a connected touch display (if available).
  // Must be called on the BrowserThread::UI thread.
  void TryToPlaceUiOnTouchDisplay();

 private:
  // Calls MoveToTouchDisplay() if touch device list is ready, otherwise adds an
  // observer that calls MoveToTouchDisplay() once ready.
  void MaybeMoveToTouchDisplay();

  void MoveToTouchDisplay();

  // ui::InputDeviceEventObserver:
  void OnTouchDeviceAssociationChanged() override;
  void OnTouchscreenDeviceConfigurationChanged() override;
  void OnDeviceListsComplete() override;

  ScopedObserver<ui::InputDeviceManager, ui::InputDeviceEventObserver>
      scoped_observer_;

  base::WeakPtrFactory<OobeDisplayChooser> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OobeDisplayChooser);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_OOBE_DISPLAY_CHOOSER_H_
