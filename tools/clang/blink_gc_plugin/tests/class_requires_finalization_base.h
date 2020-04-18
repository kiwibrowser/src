// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLASS_REQUIRES_FINALIZATION_BASE_H_
#define CLASS_REQUIRES_FINALIZATION_BASE_H_

#include "heap/stubs.h"

namespace blink {

class A : public GarbageCollected<A> {
public:
    virtual void Trace(Visitor*) {}
};

class B {
public:
    ~B() { /* user-declared, thus, non-trivial */ }
};

// Second base class needs finalization.
class NeedsFinalizer : public A, public B {
public:
    void Trace(Visitor*);
};

// Base does not need finalization.
class DoesNotNeedFinalizer : public A {
public:
    void Trace(Visitor*);
};

}

#endif
