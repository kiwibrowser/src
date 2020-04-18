// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.banners;

import android.graphics.Bitmap;
import android.os.Looper;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNIAdditionalImport;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.webapps.AddToHomescreenDialog;

/**
 * Handles the promotion and installation of an app specified by the current web page. This object
 * is created by and owned by the native AppBannerUiDelegate.
 */
@JNINamespace("banners")
@JNIAdditionalImport(InstallerDelegate.class)
public class AppBannerUiDelegateAndroid
        implements AddToHomescreenDialog.Delegate, InstallerDelegate.Observer {
    /** Pointer to the native AppBannerUiDelegateAndroid. */
    private long mNativePointer;

    /** Delegate which does the actual monitoring of an in-progress installation. */
    private InstallerDelegate mInstallerDelegate;

    private Tab mTab;

    private AddToHomescreenDialog mDialog;

    private AppBannerUiDelegateAndroid(long nativePtr, Tab tab) {
        mNativePointer = nativePtr;
        mTab = tab;
    }

    @Override
    public void addToHomescreen(String title) {
        // The title is ignored for app banners as we respect the developer-provided title.
        nativeAddToHomescreen(mNativePointer);
    }

    @Override
    public void onDialogCancelled() {
        nativeOnUiCancelled(mNativePointer);
    }

    @Override
    public void onNativeAppDetailsRequested() {
        nativeShowNativeAppDetails(mNativePointer);
    }

    @Override
    public void onDialogDismissed() {
        mDialog = null;
        mInstallerDelegate = null;
    }

    @Override
    public void onInstallIntentCompleted(InstallerDelegate delegate, boolean isInstalling) {
        // Do nothing.
    }

    @Override
    public void onInstallFinished(InstallerDelegate delegate, boolean success) {
        // Do nothing.
    }

    @Override
    public void onApplicationStateChanged(InstallerDelegate delegate, int newState) {
        // Do nothing.
    }

    /** Creates the installer delegate with the specified observer. */
    @CalledByNative
    public void createInstallerDelegate(InstallerDelegate.Observer observer) {
        mInstallerDelegate = new InstallerDelegate(Looper.getMainLooper(), observer);
    }

    @CalledByNative
    private AddToHomescreenDialog getDialogForTesting() {
        return mDialog;
    }

    @CalledByNative
    private void destroy() {
        if (mInstallerDelegate != null) {
            mInstallerDelegate.destroy();
        }
        mInstallerDelegate = null;
        mNativePointer = 0;
    }

    @CalledByNative
    private boolean installOrOpenNativeApp(AppData appData, String referrer) {
        return mInstallerDelegate.installOrOpenNativeApp(mTab, appData, referrer);
    }

    @CalledByNative
    private void showAppDetails(AppData appData) {
        mTab.getWindowAndroid().showIntent(appData.detailsIntent(), null, null);
    }

    @CalledByNative
    private boolean showNativeAppDialog(String title, Bitmap iconBitmap, AppData appData) {
        createInstallerDelegate(this);
        mDialog = new AddToHomescreenDialog(mTab.getActivity(), this);
        mDialog.show();
        mDialog.onUserTitleAvailable(title, appData.installButtonText(), appData.rating());
        mDialog.onIconAvailable(iconBitmap);
        return true;
    }

    @CalledByNative
    private boolean showWebAppDialog(String title, Bitmap iconBitmap, String url) {
        mDialog = new AddToHomescreenDialog(mTab.getActivity(), this);
        mDialog.show();
        mDialog.onUserTitleAvailable(title, url, true /* isWebapp */);
        mDialog.onIconAvailable(iconBitmap);
        return true;
    }

    @CalledByNative
    private int determineInstallState(String packageName) {
        return mInstallerDelegate.determineInstallState(packageName);
    }

    @CalledByNative
    private static AppBannerUiDelegateAndroid create(long nativePtr, Tab tab) {
        return new AppBannerUiDelegateAndroid(nativePtr, tab);
    }

    private native void nativeAddToHomescreen(long nativeAppBannerUiDelegateAndroid);
    private native void nativeOnUiCancelled(long nativeAppBannerUiDelegateAndroid);
    private native void nativeShowNativeAppDetails(long nativeAppBannerUiDelegateAndroid);
}
