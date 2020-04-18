// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/view_util.h"

#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/textfield/textfield.h"

namespace autofill {

views::Textfield* CreateCvcTextfield() {
  views::Textfield* textfield = new views::Textfield();
  textfield->set_placeholder_text(
      l10n_util::GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_CVC));
  textfield->SetDefaultWidthInChars(8);
  textfield->SetTextInputType(ui::TextInputType::TEXT_INPUT_TYPE_NUMBER);
  return textfield;
}

}  // namespace autofill
