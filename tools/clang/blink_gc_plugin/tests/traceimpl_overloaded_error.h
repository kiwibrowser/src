// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRACEIMPL_OVERLOADED_ERROR_H_
#define TRACEIMPL_OVERLOADED_ERROR_H_

#include "heap/stubs.h"

namespace blink {

class X : public GarbageCollected<X> {
 public:
  void Trace(Visitor*) {}
};

class InlinedBase : public GarbageCollected<InlinedBase> {
 public:
  virtual void Trace(Visitor* visitor) {
    // Missing visitor->Trace(x_base_).
  }

 private:
  Member<X> x_base_;
};

class InlinedDerived : public InlinedBase {
 public:
  void Trace(Visitor* visitor) override {
    // Missing visitor->Trace(x_derived_) and InlinedBase::Trace(visitor).
  }

 private:
  Member<X> x_derived_;
};

class ExternBase : public GarbageCollected<ExternBase> {
 public:
  virtual void Trace(Visitor*);

 private:
  Member<X> x_base_;
};

class ExternDerived : public ExternBase {
 public:
  void Trace(Visitor*) override;

 private:
  Member<X> x_derived_;
};

}

#endif  // TRACEIMPL_OVERLOADED_ERROR_H_
