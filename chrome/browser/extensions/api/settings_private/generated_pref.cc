// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_private/generated_pref.h"

namespace extensions {
namespace settings_private {

GeneratedPref::Observer::Observer() = default;
GeneratedPref::Observer::~Observer() = default;

GeneratedPref::GeneratedPref() = default;
GeneratedPref::~GeneratedPref() = default;

void GeneratedPref::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void GeneratedPref::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void GeneratedPref::NotifyObservers(const std::string& pref_name) {
  for (Observer& observer : observers_)
    observer.OnGeneratedPrefChanged(pref_name);
}

}  // namespace settings_private
}  // namespace extensions
