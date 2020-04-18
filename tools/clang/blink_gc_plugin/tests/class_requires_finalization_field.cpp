// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "class_requires_finalization_field.h"

namespace blink {

void NeedsFinalizer::Trace(Visitor* visitor)
{
    visitor->Trace(m_as);
    A::Trace(visitor);
}

void AlsoNeedsFinalizer::Trace(Visitor* visitor)
{
    visitor->Trace(m_bs);
    A::Trace(visitor);
}

void DoesNotNeedFinalizer::Trace(Visitor* visitor)
{
    visitor->Trace(m_bs);
    A::Trace(visitor);
}

void AlsoDoesNotNeedFinalizer::Trace(Visitor* visitor)
{
    visitor->Trace(m_as);
    visitor->Trace(m_cs);
    A::Trace(visitor);
}

}
