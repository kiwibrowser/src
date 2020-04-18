// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting;

/** The parameter for an InputModeChanged event. */
public final class InputModeChangedEventParameter {
    public final Desktop.InputMode inputMode;
    public final CapabilityManager.HostCapability hostCapability;

    public InputModeChangedEventParameter(Desktop.InputMode inputMode,
                                          CapabilityManager.HostCapability hostCapability) {
        this.inputMode = inputMode;
        this.hostCapability = hostCapability;
    }
}
