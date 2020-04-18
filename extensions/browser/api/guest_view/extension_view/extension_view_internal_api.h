// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_GUEST_VIEW_EXTENSION_VIEW_EXTENSION_VIEW_INTERNAL_API_H_
#define EXTENSIONS_BROWSER_API_GUEST_VIEW_EXTENSION_VIEW_EXTENSION_VIEW_INTERNAL_API_H_

#include "base/macros.h"
#include "extensions/browser/api/execute_code_function.h"
#include "extensions/browser/extension_function.h"
#include "extensions/browser/guest_view/extension_view/extension_view_guest.h"

namespace extensions {

// An abstract base class for async extensionview APIs. It does a process ID
// check in PreRunValidation.
class ExtensionViewInternalExtensionFunction
    : public UIThreadExtensionFunction {
 public:
  ExtensionViewInternalExtensionFunction() {}

 protected:
  ~ExtensionViewInternalExtensionFunction() override {}

  // ExtensionFunction implementation.
  bool PreRunValidation(std::string* error) final;

  ExtensionViewGuest* guest_ = nullptr;
};

// Attempts to load a src into the extensionview.
class ExtensionViewInternalLoadSrcFunction
    : public ExtensionViewInternalExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("extensionViewInternal.loadSrc",
                             EXTENSIONVIEWINTERNAL_LOADSRC);
  ExtensionViewInternalLoadSrcFunction() {}

 protected:
  ~ExtensionViewInternalLoadSrcFunction() override {}

 private:
  // UIThreadExtensionFunction implementation.
  ResponseAction Run() final;

  DISALLOW_COPY_AND_ASSIGN(ExtensionViewInternalLoadSrcFunction);
};

// Parses a src and determines whether or not it is valid.
class ExtensionViewInternalParseSrcFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("extensionViewInternal.parseSrc",
                             EXTENSIONVIEWINTERNAL_PARSESRC);
  ExtensionViewInternalParseSrcFunction() {}

 protected:
  ~ExtensionViewInternalParseSrcFunction() override {}

 private:
  // UIThreadExtensionFunction implementation.
  ResponseAction Run() final;

  DISALLOW_COPY_AND_ASSIGN(ExtensionViewInternalParseSrcFunction);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_GUEST_VIEW_EXTENSION_VIEW_EXTENSION_VIEW_INTERNAL_API_H_
