// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_RESET_METRICS_H_
#define CHROME_BROWSER_CHROMEOS_RESET_METRICS_H_

namespace chromeos {
namespace reset {

enum DialogViewType {

  // User invoked the dialog from options page.
  DIALOG_FROM_OPTIONS,

  // Invoked with shortcut. Confirming form for powerwash.
  DIALOG_SHORTCUT_CONFIRMING_POWERWASH_ONLY,

  // Invoked with shortcut. Confirming form for powerwash and rollback.
  DIALOG_SHORTCUT_CONFIRMING_POWERWASH_AND_ROLLBACK,

  // Invoked with shortcut. Offering form, rollback option set.
  DIALOG_SHORTCUT_OFFERING_ROLLBACK_UNAVAILABLE,

  // Invoked with shortcut. Offering form, rollback option not set.
  DIALOG_SHORTCUT_OFFERING_ROLLBACK_AVAILABLE,

  // Invoked with shortcut. Requesting restart form.
  DIALOG_SHORTCUT_RESTART_REQUIRED,

  // Must be last enum element.
  DIALOG_VIEW_TYPE_SIZE
};

}  // namespace reset
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_RESET_METRICS_H_
