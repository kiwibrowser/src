// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.banners.InstallerDelegate;

/**
 * Handles the promotion and installation of an app specified by the current web page. This object
 * is created by and owned by the native AppBannerInfoBarDelegateAndroid.
 */
@JNINamespace("banners")
public class AppBannerInfoBarDelegateAndroid implements InstallerDelegate.Observer {
    /** Pointer to the native AppBannerInfoBarDelegateAndroid. */
    private long mNativePointer;

    private AppBannerInfoBarDelegateAndroid(long nativePtr) {
        mNativePointer = nativePtr;
    }

    @Override
    public void onInstallIntentCompleted(InstallerDelegate delegate, boolean isInstalling) {
        if (mNativePointer != 0) nativeOnInstallIntentReturned(mNativePointer, isInstalling);
    }

    @Override
    public void onInstallFinished(InstallerDelegate delegate, boolean success) {
        if (mNativePointer != 0) nativeOnInstallFinished(mNativePointer, success);
    }

    @Override
    public void onApplicationStateChanged(InstallerDelegate delegate, int newState) {
        if (mNativePointer != 0) nativeUpdateInstallState(mNativePointer);
    }

    @CalledByNative
    private void destroy() {
        mNativePointer = 0;
    }

    @CalledByNative
    private static AppBannerInfoBarDelegateAndroid create(long nativePtr) {
        return new AppBannerInfoBarDelegateAndroid(nativePtr);
    }

    private native void nativeOnInstallIntentReturned(
            long nativeAppBannerInfoBarDelegateAndroid, boolean isInstalling);
    private native void nativeOnInstallFinished(
            long nativeAppBannerInfoBarDelegateAndroid, boolean success);
    private native void nativeUpdateInstallState(long nativeAppBannerInfoBarDelegateAndroid);
}
