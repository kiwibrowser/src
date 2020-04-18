// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SPELLCHECK_RENDERER_CUSTOM_DICTIONARY_ENGINE_H_
#define COMPONENTS_SPELLCHECK_RENDERER_CUSTOM_DICTIONARY_ENGINE_H_

#include <set>
#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"

// Custom spellcheck dictionary. Words in this dictionary are always correctly
// spelled. Words that are not in this dictionary may or may not be correctly
// spelled.
class CustomDictionaryEngine {
 public:
  CustomDictionaryEngine();
  ~CustomDictionaryEngine();

  // Initialize the custom dictionary engine.
  void Init(const std::set<std::string>& words);

  // Spellcheck |text|. Assumes that another spelling engine has set
  // |misspelling_start| and |misspelling_len| to indicate a misspelling.
  // Returns true if there are no misspellings, otherwise returns false.
  bool SpellCheckWord(const base::string16& text,
                      int misspelling_start,
                      int misspelling_len);

  // Update custom dictionary words.
  void OnCustomDictionaryChanged(const std::set<std::string>& words_added,
                                 const std::set<std::string>& words_removed);

 private:
  // Correctly spelled words.
  std::set<base::string16> dictionary_;

  DISALLOW_COPY_AND_ASSIGN(CustomDictionaryEngine);
};

#endif  // COMPONENTS_SPELLCHECK_RENDERER_CUSTOM_DICTIONARY_ENGINE_H_
