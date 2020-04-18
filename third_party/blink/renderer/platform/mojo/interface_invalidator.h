// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_INTERFACE_INVALIDATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_INTERFACE_INVALIDATOR_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

// Notifies weak interface bindings to be invalidated when this object is
// destroyed.
class PLATFORM_EXPORT InterfaceInvalidator {
 public:
  InterfaceInvalidator();
  ~InterfaceInvalidator();

  class Observer {
   public:
    virtual void OnInvalidate() = 0;
  };

  void AddObserver(Observer*);
  void RemoveObserver(const Observer*);

  base::WeakPtr<InterfaceInvalidator> GetWeakPtr();

 private:
  void NotifyInvalidate();

  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<InterfaceInvalidator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InterfaceInvalidator);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_INTERFACE_INVALIDATOR_H_
