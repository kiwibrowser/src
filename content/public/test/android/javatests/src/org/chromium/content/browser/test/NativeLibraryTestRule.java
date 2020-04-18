// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test;

import org.junit.Assert;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.PathUtils;
import org.chromium.base.ResourceExtractor;
import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.content.browser.BrowserStartupController;


/**
 * TestRule that adds support for loading and dealing with native libraries.
 *
 * NativeLibraryTestRule does not interact with any Activity.
 */
public class NativeLibraryTestRule implements TestRule {
    private static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "content";

    /**
     * Loads the native library on the activity UI thread (must not be called from the UI thread).
     */
    public void loadNativeLibraryNoBrowserProcess() {
        handleNativeInitialization(false);
    }

    /**
     * Loads the native library on the activity UI thread (must not be called from the UI thread).
     * After loading the library, this will initialize the browser process.
     */
    public void loadNativeLibraryAndInitBrowserProcess() {
        handleNativeInitialization(true);
    }

    private void handleNativeInitialization(final boolean initBrowserProcess) {
        Assert.assertFalse(ThreadUtils.runningOnUiThread());

        PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);

        // LibraryLoader is not in general multithreaded; as other InstrumentationTestCase code
        // (specifically, ChromeBrowserProvider) uses it from the main thread we must do
        // likewise.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                nativeInitialization(initBrowserProcess);
            }
        });
    }

    private void nativeInitialization(boolean initBrowserProcess) {
        if (initBrowserProcess) {
            try {
                // Extract compressed resource paks.
                ResourceExtractor resourceExtractor = ResourceExtractor.get();
                resourceExtractor.startExtractingResources();
                resourceExtractor.waitForCompletion();

                BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                        .startBrowserProcessesSync(false);
            } catch (ProcessInitException e) {
                throw new Error(e);
            }
        } else {
            try {
                LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
            } catch (ProcessInitException e) {
                throw new Error(e);
            }
        }
    }

    @Override
    public Statement apply(Statement base, Description description) {
        return base;
    }
}
