// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/dictionary_load_observer.h"

DictionaryLoadObserver::DictionaryLoadObserver(const base::Closure& quit_task)
    : quit_task_(quit_task) {
}

DictionaryLoadObserver::~DictionaryLoadObserver() {
}

void DictionaryLoadObserver::OnCustomDictionaryLoaded() {
  quit_task_.Run();
}

void DictionaryLoadObserver::OnCustomDictionaryChanged(
    const SpellcheckCustomDictionary::Change& dictionary_change) {
}
