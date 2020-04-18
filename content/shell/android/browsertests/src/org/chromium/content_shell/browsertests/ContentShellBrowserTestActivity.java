// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell.browsertests;

import android.view.Window;
import android.view.WindowManager;

import org.chromium.base.Log;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.content.browser.BrowserStartupController;
import org.chromium.content_shell.ShellManager;
import org.chromium.native_test.NativeBrowserTestActivity;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.WindowAndroid;

/** An Activity base class for running browser tests against ContentShell. */
public abstract class ContentShellBrowserTestActivity extends NativeBrowserTestActivity {

    private static final String TAG = "cr.native_test";

    private ShellManager mShellManager;
    private WindowAndroid mWindowAndroid;

    /**
     * Initializes the browser process.
     *
     * This generally includes loading native libraries and switching to the native command line,
     * among other things.
     */
    @Override
    protected void initializeBrowserProcess() {
        try {
            LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
        } catch (ProcessInitException e) {
            Log.e(TAG, "Cannot load content_browsertests.", e);
            System.exit(-1);
        }
        BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                .initChromiumBrowserProcessForTests();

        setContentView(getTestActivityViewId());
        mShellManager = (ShellManager) findViewById(getShellManagerViewId());
        mWindowAndroid = new ActivityWindowAndroid(this);
        mShellManager.setWindow(mWindowAndroid);

        Window wind = this.getWindow();
        wind.addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        wind.addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
        wind.addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
    }

    protected abstract int getTestActivityViewId();

    protected abstract int getShellManagerViewId();
}
