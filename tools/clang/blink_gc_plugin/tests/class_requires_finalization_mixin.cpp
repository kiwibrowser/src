// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "class_requires_finalization_mixin.h"

namespace blink {

void MixinFinalizable::Trace(Visitor* visitor)
{
    visitor->Trace(m_onHeap);
}

void MixinNotFinalizable::Trace(Visitor* visitor)
{
    visitor->Trace(m_onHeap);
}

void NeedsFinalizer::Trace(Visitor* visitor)
{
    visitor->Trace(m_obj);
    MixinFinalizable::Trace(visitor);
}

void HasFinalizer::Trace(Visitor* visitor)
{
    visitor->Trace(m_obj);
    MixinFinalizable::Trace(visitor);
}

void NeedsNoFinalization::Trace(Visitor* visitor)
{
    visitor->Trace(m_obj);
    MixinNotFinalizable::Trace(visitor);
}

}
