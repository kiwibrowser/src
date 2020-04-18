// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "finalize_after_dispatch.h"

namespace blink {

static B* toB(A* a) { return static_cast<B*>(a); }

void A::Trace(Visitor* visitor)
{
    switch (m_type) {
    case TB:
        toB(this)->TraceAfterDispatch(visitor);
        break;
    case TC:
        static_cast<C*>(this)->TraceAfterDispatch(visitor);
        break;
    case TD:
        static_cast<D*>(this)->TraceAfterDispatch(visitor);
        break;
    }
}

void A::TraceAfterDispatch(Visitor* visitor)
{
}

void A::FinalizeGarbageCollectedObject()
{
    switch (m_type) {
    case TB:
        toB(this)->~B();
        break;
    case TC:
        static_cast<C*>(this)->~C();
        break;
    case TD:
        // Missing static_cast<D*>(this)->~D();
        break;
    }
}

void B::TraceAfterDispatch(Visitor* visitor)
{
    visitor->Trace(m_a);
    A::TraceAfterDispatch(visitor);
}

void C::TraceAfterDispatch(Visitor* visitor)
{
    visitor->Trace(m_a);
    A::TraceAfterDispatch(visitor);
}

void D::TraceAfterDispatch(Visitor* visitor)
{
    visitor->Trace(m_a);
    Abstract::TraceAfterDispatch(visitor);
}

}
