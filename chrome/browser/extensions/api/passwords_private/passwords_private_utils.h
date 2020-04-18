// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_PASSWORDS_PRIVATE_PASSWORDS_PRIVATE_UTILS_H_
#define CHROME_BROWSER_EXTENSIONS_API_PASSWORDS_PRIVATE_PASSWORDS_PRIVATE_UTILS_H_

#include "chrome/common/extensions/api/passwords_private.h"

namespace autofill {
struct PasswordForm;
}

namespace extensions {

// Obtains a collection of URLs from the passed in form. This includes an origin
// URL used for internal logic, a human friendly string shown to the user as
// well as a URL that is linked to.
api::passwords_private::UrlCollection CreateUrlCollectionFromForm(
    const autofill::PasswordForm& form);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_PASSWORDS_PRIVATE_PASSWORDS_PRIVATE_UTILS_H_
