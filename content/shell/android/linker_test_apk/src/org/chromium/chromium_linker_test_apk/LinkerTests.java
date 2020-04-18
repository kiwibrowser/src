// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromium_linker_test_apk;

import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.library_loader.Linker;

/**
 * A class that is only used in linker test APK to perform runtime checks
 * in the current process.
 */
@JNINamespace("content")
public class LinkerTests implements Linker.TestRunner {
    private static final String TAG = "LinkerTest";

    public LinkerTests() {}

    @Override
    public boolean runChecks(int memoryDeviceConfig,
                             boolean isBrowserProcess) {
        boolean checkSharedRelro;
        if (isBrowserProcess) {
            // LegacyLinker may share RELROs in the browser.
            switch (Linker.BROWSER_SHARED_RELRO_CONFIG) {
                case Linker.BROWSER_SHARED_RELRO_CONFIG_NEVER:
                    checkSharedRelro = false;
                    break;
                case Linker.BROWSER_SHARED_RELRO_CONFIG_LOW_RAM_ONLY:
                    // A shared RELRO should only be used on low-end devices.
                    checkSharedRelro = (memoryDeviceConfig == Linker.MEMORY_DEVICE_CONFIG_LOW);
                    break;
                case Linker.BROWSER_SHARED_RELRO_CONFIG_ALWAYS:
                    // Always check for a shared RELRO.
                    checkSharedRelro = true;
                    break;
                default:
                    Log.e(TAG,
                            "Invalid shared RELRO linker configuration: "
                                    + Linker.BROWSER_SHARED_RELRO_CONFIG);
                    return false;
            }
        } else {
            // Service processes should always use a shared RELRO section.
            checkSharedRelro = true;
        }

        if (checkSharedRelro) {
            return nativeCheckForSharedRelros(isBrowserProcess);
        } else {
            return nativeCheckForNoSharedRelros(isBrowserProcess);
        }
    }

    // Check that there are shared RELRO sections in the current process,
    // and that they are properly mapped read-only. Returns true on success.
    private static native boolean nativeCheckForSharedRelros(boolean isBrowserProcess);

    // Check that there are no shared RELRO sections in the current process,
    // return true on success.
    private static native boolean nativeCheckForNoSharedRelros(boolean isBrowserProcess);
}
