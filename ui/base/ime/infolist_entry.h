// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INFOLIST_ENTRY_H_
#define UI_BASE_IME_INFOLIST_ENTRY_H_

#include "base/strings/string16.h"
#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {

// The data model of infolist window.
struct UI_BASE_IME_EXPORT InfolistEntry {
  base::string16 title;
  base::string16 body;
  bool highlighted;

  InfolistEntry(const base::string16& title, const base::string16& body);
  bool operator==(const InfolistEntry& entry) const;
  bool operator!=(const InfolistEntry& entry) const;
};

}  // namespace ui

#endif  // UI_BASE_IME_INFOLIST_ENTRY_H_
