// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_API_EXTENSION_ACTION_ACTION_INFO_H_
#define CHROME_COMMON_EXTENSIONS_API_EXTENSION_ACTION_ACTION_INFO_H_

#include <memory>
#include <string>

#include "base/strings/string16.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
}

namespace extensions {

class Extension;

struct ActionInfo {
  ActionInfo();
  ActionInfo(const ActionInfo& other);
  ~ActionInfo();

  // The types of extension actions.
  enum Type {
    TYPE_BROWSER,
    TYPE_PAGE,
    TYPE_SYSTEM_INDICATOR,
  };

  enum DefaultState {
    STATE_ENABLED,
    STATE_DISABLED,
  };

  // Loads an ActionInfo from the given DictionaryValue.
  static std::unique_ptr<ActionInfo> Load(const Extension* extension,
                                          const base::DictionaryValue* dict,
                                          base::string16* error);

  // Returns the extension's browser action, if any.
  static const ActionInfo* GetBrowserActionInfo(const Extension* extension);

  // Returns the extension's page action, if any.
  static const ActionInfo* GetPageActionInfo(const Extension* extension);

  // Returns the extension's system indicator, if any.
  static const ActionInfo* GetSystemIndicatorInfo(const Extension* extension);

  // Sets the extension's action. |extension| takes ownership of |info|.
  static void SetExtensionActionInfo(Extension* extension, ActionInfo* info);

  // Sets the extension's browser action. |extension| takes ownership of |info|.
  static void SetBrowserActionInfo(Extension* extension, ActionInfo* info);

  // Sets the extension's page action. |extension| takes ownership of |info|.
  static void SetPageActionInfo(Extension* extension, ActionInfo* info);

  // Sets the extension's system indicator. |extension| takes ownership of
  // |info|.
  static void SetSystemIndicatorInfo(Extension* extension, ActionInfo* info);

  // Returns true if the extension needs a verbose install message because
  // of its page action.
  static bool IsVerboseInstallMessage(const Extension* extension);

  // Empty implies the key wasn't present.
  ExtensionIconSet default_icon;
  std::string default_title;
  GURL default_popup_url;
  // Specifies if the action applies to all web pages ("enabled") or
  // only specific pages ("disabled"). Only applies to the "action" key.
  DefaultState default_state;
  // Whether or not this action was synthesized to force visibility.
  bool synthesized;
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_API_EXTENSION_ACTION_ACTION_INFO_H_
