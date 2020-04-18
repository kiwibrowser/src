// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_media_recorder_handler.h"

// This file just includes WebMediaPlayerClient.h, to make sure
// MSVC compiler does not fail linking with LNK2019 due to unresolved
// constructor/destructor and should be in Source/platform/exported.
