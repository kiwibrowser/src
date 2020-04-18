// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.Activity;
import android.graphics.Bitmap;
import android.text.TextUtils;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.WebContents;

/**
 * Manages the add to home screen process. Coordinates the native-side data fetching, and owns
 * a dialog prompting the user to confirm the action (and potentially supply a title).
 */
public class AddToHomescreenManager implements AddToHomescreenDialog.Delegate {
    protected final Activity mActivity;
    protected final Tab mTab;

    protected AddToHomescreenDialog mDialog;
    private long mNativeAddToHomescreenManager;

    public AddToHomescreenManager(Activity activity, Tab tab) {
        mActivity = activity;
        mTab = tab;
    }

    /**
     * Starts the add to home screen process. Creates the C++ AddToHomescreenManager, which fetches
     * the data needed for add to home screen, and informs this object when data is available and
     * when the dialog can be shown.
     */
    public void start() {
        // Don't start if we've already started or if there is no visible URL to add.
        if (mNativeAddToHomescreenManager != 0 || TextUtils.isEmpty(mTab.getUrl())) return;

        mNativeAddToHomescreenManager = nativeInitializeAndStart(mTab.getWebContents());
    }

    /**
     * Puts the object in a state where it is safe to be destroyed.
     */
    public void destroy() {
        mDialog = null;
        if (mNativeAddToHomescreenManager == 0) return;

        nativeDestroy(mNativeAddToHomescreenManager);
        mNativeAddToHomescreenManager = 0;
    }

    /**
     * Adds a shortcut for the current Tab. Must not be called unless start() has been called.
     * @param userRequestedTitle Title of the shortcut displayed on the homescreen.
     */
    @Override
    public void addToHomescreen(String userRequestedTitle) {
        assert mNativeAddToHomescreenManager != 0;

        nativeAddToHomescreen(mNativeAddToHomescreenManager, userRequestedTitle);
    }

    @Override
    public void onDialogCancelled() {
        // Do nothing.
    }

    @Override
    public void onNativeAppDetailsRequested() {
        // This should never be called.
        assert false;
    }

    @Override
    /**
     * Destroys this object once the dialog has been dismissed.
     */
    public void onDialogDismissed() {
        destroy();
    }

    /**
     * Shows alert to prompt user for name of home screen shortcut.
     */
    @CalledByNative
    public void showDialog() {
        mDialog = new AddToHomescreenDialog(mActivity, this);
        mDialog.show();
    }

    @CalledByNative
    private void onUserTitleAvailable(String title, String url, boolean isWebapp) {
        // Users may edit the title of bookmark shortcuts, but we respect web app names and do not
        // let users change them.
        mDialog.onUserTitleAvailable(title, url, isWebapp);
    }

    @CalledByNative
    private void onIconAvailable(Bitmap icon) {
        mDialog.onIconAvailable(icon);
    }

    private native long nativeInitializeAndStart(WebContents webContents);
    private native void nativeAddToHomescreen(
            long nativeAddToHomescreenManager, String userRequestedTitle);
    private native void nativeDestroy(long nativeAddToHomescreenManager);
}
