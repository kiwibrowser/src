// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_DECLARATIVE_NET_REQUEST_API_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_DECLARATIVE_NET_REQUEST_API_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

// Helper base class to update the set of whitelisted pages.
class DeclarativeNetRequestUpdateWhitelistedPagesFunction
    : public UIThreadExtensionFunction {
 protected:
  enum class Action {
    ADD,     // Add whitelisted pages.
    REMOVE,  // Remove whitelisted pages.
  };
  DeclarativeNetRequestUpdateWhitelistedPagesFunction();
  ~DeclarativeNetRequestUpdateWhitelistedPagesFunction() override;

  // Updates the set of whitelisted pages for the extension.
  ExtensionFunction::ResponseAction UpdateWhitelistedPages(
      const std::vector<std::string>& patterns,
      Action action);

  // ExtensionFunction override:
  bool PreRunValidation(std::string* error) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestUpdateWhitelistedPagesFunction);
};

// Implements the "declarativeNetRequest.addWhitelistedPages" extension
// function.
class DeclarativeNetRequestAddWhitelistedPagesFunction
    : public DeclarativeNetRequestUpdateWhitelistedPagesFunction {
 public:
  DeclarativeNetRequestAddWhitelistedPagesFunction();
  DECLARE_EXTENSION_FUNCTION("declarativeNetRequest.addWhitelistedPages",
                             DECLARATIVENETREQUEST_ADDWHITELISTEDPAGES);

 protected:
  ~DeclarativeNetRequestAddWhitelistedPagesFunction() override;

  // ExtensionFunction override:
  ExtensionFunction::ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestAddWhitelistedPagesFunction);
};

// Implements the "declarativeNetRequest.removeWhitelistedPages" extension
// function.
class DeclarativeNetRequestRemoveWhitelistedPagesFunction
    : public DeclarativeNetRequestUpdateWhitelistedPagesFunction {
 public:
  DeclarativeNetRequestRemoveWhitelistedPagesFunction();
  DECLARE_EXTENSION_FUNCTION("declarativeNetRequest.removeWhitelistedPages",
                             DECLARATIVENETREQUEST_REMOVEWHITELISTEDPAGES);

 protected:
  ~DeclarativeNetRequestRemoveWhitelistedPagesFunction() override;

  // ExtensionFunction override:
  ExtensionFunction::ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestRemoveWhitelistedPagesFunction);
};

// Implements the "declarativeNetRequest.getWhitelistedPages" extension
// function.
class DeclarativeNetRequestGetWhitelistedPagesFunction
    : public UIThreadExtensionFunction {
 public:
  DeclarativeNetRequestGetWhitelistedPagesFunction();
  DECLARE_EXTENSION_FUNCTION("declarativeNetRequest.getWhitelistedPages",
                             DECLARATIVENETREQUEST_GETWHITELISTEDPAGES);

 protected:
  ~DeclarativeNetRequestGetWhitelistedPagesFunction() override;

  // ExtensionFunction overrides:
  bool PreRunValidation(std::string* error) override;
  ExtensionFunction::ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestGetWhitelistedPagesFunction);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_DECLARATIVE_NET_REQUEST_API_H_
