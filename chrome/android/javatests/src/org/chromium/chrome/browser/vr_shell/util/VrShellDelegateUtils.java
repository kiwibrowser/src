// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.util;

import org.junit.Assert;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.vr_shell.TestVrShellDelegate;
import org.chromium.chrome.browser.vr_shell.VrCoreInfo;
import org.chromium.chrome.browser.vr_shell.mock.MockVrCoreVersionCheckerImpl;

import java.util.concurrent.atomic.AtomicReference;

/**
 * Class containing utility functions for interacting with VrShellDelegate
 * during tests.
 */
public class VrShellDelegateUtils {
    /**
     * Retrieves the current VrShellDelegate instance from the UI thread.
     * This is necessary in case acquiring the instance causes the delegate
     * to be constructed, which must happen on the UI thread.
     */
    public static TestVrShellDelegate getDelegateInstance() {
        final AtomicReference<TestVrShellDelegate> delegate =
                new AtomicReference<TestVrShellDelegate>();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                delegate.set(TestVrShellDelegate.getInstance());
            }
        });
        return delegate.get();
    }

    /**
     * Creates and sets a MockVrCoreVersionCheckerImpl as the VrShellDelegate's
     * VrCoreVersionChecker instance.
     * @param compatibility An int corresponding to a VrCoreCompatibility value that the mock
     *     version checker will return.
     * @return The MockVrCoreVersionCheckerImpl that was set as VrShellDelegate's
     *     VrCoreVersionChecker instance.
     */
    public static MockVrCoreVersionCheckerImpl setVrCoreCompatibility(int compatibility) {
        final MockVrCoreVersionCheckerImpl mockChecker = new MockVrCoreVersionCheckerImpl();
        mockChecker.setMockReturnValue(new VrCoreInfo(null, compatibility));
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                VrShellDelegateUtils.getDelegateInstance().overrideVrCoreVersionCheckerForTesting(
                        mockChecker);
            }
        });
        Assert.assertEquals(compatibility, mockChecker.getLastReturnValue().compatibility);
        return mockChecker;
    }
}
