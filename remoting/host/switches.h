// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_SWITCHES_H_
#define REMOTING_HOST_SWITCHES_H_

#include "build/build_config.h"

namespace remoting {

// "--elevate=<binary>" requests |binary| to be launched elevated (possibly
// causing a UAC prompt).
extern const char kElevateSwitchName[];

// "--help" prints the usage message.
extern const char kHelpSwitchName[];

// Used to specify the type of the process. kProcessType* constants specify
// possible values.
extern const char kProcessTypeSwitchName[];

// "--?" prints the usage message.
extern const char kQuestionSwitchName[];

// The command line switch used to get version of the daemon.
extern const char kVersionSwitchName[];

// Values for kProcessTypeSwitchName.
extern const char kProcessTypeController[];
extern const char kProcessTypeDaemon[];
extern const char kProcessTypeDesktop[];
extern const char kProcessTypeHost[];
extern const char kProcessTypeRdpDesktopSession[];
extern const char kProcessTypeEvaluateCapability[];

extern const char kEvaluateCapabilitySwitchName[];

// Values for kEvaluateCapabilitySwitchName.
#if defined(OS_WIN)
// Executes EvaluateD3D() function.
extern const char kEvaluateD3D[];
#endif

// Used to pass the HWND for the parent process to a child process.
extern const char kParentWindowSwitchName[];

// Name of the pipe used to communicate from the parent to the child process.
extern const char kInputSwitchName[];

// Name of the pipe used to communicate from the child to the parent process.
extern const char kOutputSwitchName[];

// Token used to create a message pipe between a pair of child and parent
// processes.
extern const char kMojoPipeToken[];

}  // namespace remoting

#endif  // REMOTING_HOST_SWITCHES_H_
