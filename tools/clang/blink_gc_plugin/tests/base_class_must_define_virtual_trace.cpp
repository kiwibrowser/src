// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base_class_must_define_virtual_trace.h"

namespace blink {

void PartDerived::Trace(Visitor* visitor)
{
}

void HeapDerived::Trace(Visitor* visitor)
{
    visitor->Trace(m_part);
}


}
