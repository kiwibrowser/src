// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is for including headers that are not included in any other .cc
// files contained within the ui/views module.  We need to include these here so
// that linker will know to include the symbols, defined by these headers, in
// the resulting dynamic library.

#include "ui/views/pointer_watcher.h"
