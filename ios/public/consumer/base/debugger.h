// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_CONSUMER_BASE_DEBUGGER_H_
#define IOS_PUBLIC_CONSUMER_BASE_DEBUGGER_H_

namespace ios {

// Returns true if the given process is being run under a debugger.
bool BeingDebugged();

}  // namespace ios

#endif  // IOS_PUBLIC_CONSUMER_BASE_DEBUGGER_H_
