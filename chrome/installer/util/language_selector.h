// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file declares a helper class for selecting a supported language from a
// set of candidates.

#ifndef CHROME_INSTALLER_UTIL_LANGUAGE_SELECTOR_H_
#define CHROME_INSTALLER_UTIL_LANGUAGE_SELECTOR_H_

#include <string>
#include <vector>

#include "base/macros.h"

namespace installer {

// A helper class for selecting a supported language from a set of candidates.
// By default, the candidates are retrieved from the operating system.
class LanguageSelector {
 public:
  // Default constructor will select from the set of languages supported by the
  // operating system.
  LanguageSelector();

  // Constructor for testing purposes.
  explicit LanguageSelector(const std::vector<std::wstring>& candidates);

  ~LanguageSelector();

  // The offset of the matched language (i.e., IDS_L10N_OFFSET_*).
  int offset() const { return offset_; }

  // The full name of the candidate language for which a match was found.
  const std::wstring& matched_candidate() const { return matched_candidate_; }

  // The name of the selected translation.
  std::wstring selected_translation() const { return GetLanguageName(offset_); }

  // Returns the name of a translation given its offset.
  static std::wstring GetLanguageName(int offset);

 private:
  typedef bool (*SelectPred_Fn)(const std::wstring&, int*);

  static bool SelectIf(const std::vector<std::wstring>& candidates,
                       SelectPred_Fn select_predicate,
                       std::wstring* matched_name, int* matched_offset);
  void DoSelect(const std::vector<std::wstring>& candidates);

  std::wstring matched_candidate_;
  int offset_;

  DISALLOW_COPY_AND_ASSIGN(LanguageSelector);
};

}  // namespace installer.

#endif  // CHROME_INSTALLER_UTIL_LANGUAGE_SELECTOR_H_
