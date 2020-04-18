// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SPELLCHECK_BROWSER_SPELLCHECK_DICTIONARY_H_
#define COMPONENTS_SPELLCHECK_BROWSER_SPELLCHECK_DICTIONARY_H_

#include "base/macros.h"

// Defines a dictionary for use in the spellchecker system and provides access
// to words within the dictionary.
class SpellcheckDictionary {
 public:
  SpellcheckDictionary() {}
  virtual ~SpellcheckDictionary() {}

  virtual void Load() = 0;

 protected:
  DISALLOW_COPY_AND_ASSIGN(SpellcheckDictionary);
};

#endif  // COMPONENTS_SPELLCHECK_BROWSER_SPELLCHECK_DICTIONARY_H_
