// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_WIN_EVALUATE_D3D_H_
#define REMOTING_HOST_WIN_EVALUATE_D3D_H_

#include <string>
#include <vector>

namespace remoting {

// Evaluates the D3D capability of the system and outputs the results into
// stdout.
// DO NOT call this method within the host process. Only call in an isolated
// child process. I.e. from EvaluateCapabilityLocally().
int EvaluateD3D();

// Evaluates the D3D capability of the system in a separate process. Returns
// true if the process succeeded. The capabilities will be stored in |result| if
// it's not nullptr.
// Note, this is not a cheap call, it uses EvaluateCapability() internally to
// spawn a new process, which may take a noticeable amount of time.
bool GetD3DCapability(std::vector<std::string>* result = nullptr);

}  // namespace remoting

#endif  // REMOTING_HOST_WIN_EVALUATE_D3D_H_
