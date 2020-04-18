// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_DATA_DELETER_H_
#define CHROME_BROWSER_EXTENSIONS_DATA_DELETER_H_

#include "base/macros.h"

class Profile;

namespace extensions {

class Extension;

class DataDeleter {
 public:
  // Starts removing data. The extension should not be running when this is
  // called. Cookies are deleted on the current thread, local storage and
  // databases/settings are deleted asynchronously on the webkit and file
  // threads, respectively. This function must be called from the UI thread.
  // This method only starts the deletion process in a fire-and-forget fashion;
  // the deletion will finish asynchronously.
  static void StartDeleting(Profile* profile, const Extension* extenion);

 private:
  DISALLOW_COPY_AND_ASSIGN(DataDeleter);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_DATA_DELETER_H_
