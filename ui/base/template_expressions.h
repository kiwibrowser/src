// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines utility functions for replacing template expressions.
// For example "Hello ${name}" could have ${name} replaced by the user's name.

#ifndef UI_BASE_TEMPLATE_EXPRESSIONS_H_
#define UI_BASE_TEMPLATE_EXPRESSIONS_H_

#include <map>
#include <string>

#include "base/strings/string_piece.h"
#include "ui/base/ui_base_export.h"

namespace base {
class DictionaryValue;
}

namespace ui {

// Map of strings for template replacement in |ReplaceTemplateExpressions|.
typedef std::map<const std::string, std::string> TemplateReplacements;

// Convert a dictionary to a replacement map. This helper function is to assist
// migration to using TemplateReplacements directly (which is preferred).
// TODO(dschuyler): remove this function by using TemplateReplacements directly.
UI_BASE_EXPORT void TemplateReplacementsFromDictionaryValue(
    const base::DictionaryValue& dictionary,
    TemplateReplacements* replacements);

// Replace $i18n*{foo} in the format string with the value for the foo key in
// |subst|.  If the key is not found in the |substitutions| that item will
// be unaltered.
UI_BASE_EXPORT std::string ReplaceTemplateExpressions(
    base::StringPiece source,
    const TemplateReplacements& replacements);

}  // namespace ui

#endif  // UI_BASE_TEMPLATE_EXPRESSIONS_H_
