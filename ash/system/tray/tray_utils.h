// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRAY_UTILS_H_
#define ASH_SYSTEM_TRAY_TRAY_UTILS_H_

namespace views {
class Label;
}

namespace ash {

// Sets up a Label properly for the tray (sets color, font etc.).
void SetupLabelForTray(views::Label* label);

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TRAY_UTILS_H_
