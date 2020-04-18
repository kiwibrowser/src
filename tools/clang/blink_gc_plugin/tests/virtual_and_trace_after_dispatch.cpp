// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "virtual_and_trace_after_dispatch.h"

namespace blink {

static B* toB(A* a) { return static_cast<B*>(a); }

void A::Trace(Visitor* visitor)
{
    switch (m_type) {
    case TB:
        toB(this)->TraceAfterDispatch(visitor);
        break;
    }
}

void A::TraceAfterDispatch(Visitor* visitor)
{
}

void B::TraceAfterDispatch(Visitor* visitor)
{
    visitor->Trace(m_a);
    A::TraceAfterDispatch(visitor);
}

}
