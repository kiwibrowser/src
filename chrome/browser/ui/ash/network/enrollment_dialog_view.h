// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_NETWORK_ENROLLMENT_DIALOG_VIEW_H_
#define CHROME_BROWSER_UI_ASH_NETWORK_ENROLLMENT_DIALOG_VIEW_H_

#include <string>

#include "ui/gfx/native_widget_types.h"

namespace chromeos {
namespace enrollment {

// Creates and shows the dialog for certificate-based network enrollment. If the
// |owning_window| is null the dialog is placed in the appropriate modal dialog
// dialog container on the primary display. Returns true if a dialog was
// successfully created.
bool CreateEnrollmentDialog(const std::string& network_id,
                            gfx::NativeWindow owning_window);

}  // namespace enrollment
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_ASH_NETWORK_ENROLLMENT_DIALOG_VIEW_H_
