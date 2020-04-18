// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.mock;

import org.chromium.chrome.browser.vr_shell.VrCoreCompatibility;
import org.chromium.chrome.browser.vr_shell.VrCoreInfo;
import org.chromium.chrome.browser.vr_shell.VrCoreVersionCheckerImpl;

/**
 * Mock version of VrCoreVersionCheckerImpl that allows setting of the return
 * value.
 */
public class MockVrCoreVersionCheckerImpl extends VrCoreVersionCheckerImpl {
    private boolean mUseActualImplementation;
    private VrCoreInfo mMockReturnValue = new VrCoreInfo(null, VrCoreCompatibility.VR_READY);
    private VrCoreInfo mLastReturnValue = null;

    @Override
    public VrCoreInfo getVrCoreInfo() {
        mLastReturnValue = mUseActualImplementation ? super.getVrCoreInfo() : mMockReturnValue;
        return mLastReturnValue;
    }

    public VrCoreInfo getLastReturnValue() {
        return mLastReturnValue;
    }

    public void setMockReturnValue(VrCoreInfo value) {
        mMockReturnValue = value;
    }

    public void setUseActualImplementation(boolean useActual) {
        mUseActualImplementation = useActual;
    }
}
