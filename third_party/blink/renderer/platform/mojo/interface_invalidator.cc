// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/mojo/interface_invalidator.h"

namespace blink {

InterfaceInvalidator::InterfaceInvalidator() : weak_factory_(this) {}

InterfaceInvalidator::~InterfaceInvalidator() {
  weak_factory_.InvalidateWeakPtrs();
  NotifyInvalidate();
}

void InterfaceInvalidator::AddObserver(Observer* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void InterfaceInvalidator::RemoveObserver(const Observer* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

base::WeakPtr<InterfaceInvalidator> InterfaceInvalidator::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void InterfaceInvalidator::NotifyInvalidate() {
  for (auto& observer : observers_)
    observer.OnInvalidate();
}

}  // namespace blink
