// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is SkOpts_hsw.cpp, stubbed out to do nothing. This is used by the
// SyzyAsan builds because the Syzygy instrumentation pipeline doesn't support
// the AVX2 and F16C instructions.

namespace SkOpts {
    void Init_hsw();
    void Init_hsw() {}
}

