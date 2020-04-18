// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.banners;

import android.content.Context;
import android.text.TextUtils;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;
import org.chromium.chrome.browser.webapps.AddToHomescreenDialog;
import org.chromium.content_public.browser.WebContents;

/**
 * Manages an AppBannerInfoBar for a Tab.
 *
 * The AppBannerManager is responsible for fetching details about native apps to display in the
 * banner. The actual observation of the WebContents (which triggers the automatic creation and
 * removal of banners, among other things) is done by the native-side AppBannerManagerAndroid.
 */
@JNINamespace("banners")
public class AppBannerManager {
    private static final String TAG = "AppBannerManager";

    /** Retrieves information about a given package. */
    private static AppDetailsDelegate sAppDetailsDelegate;

    /** Whether add to home screen is permitted by the system. */
    private static Boolean sIsSupported;

    /** Whether the tab to which this manager is attached to is permitted to show banners. */
    private boolean mIsEnabledForTab;

    /** Pointer to the native side AppBannerManager. */
    private long mNativePointer;

    /**
     * Checks if the add to home screen intent is supported.
     * @return true if add to home screen is supported, false otherwise.
     */
    public static boolean isSupported() {
        // TODO(mthiesse, https://crbug.com/840811): Support the app banner dialog in VR.
        if (VrShellDelegate.isInVr()) return false;
        if (sIsSupported == null) {
            sIsSupported = ShortcutHelper.isAddToHomeIntentSupported();
        }
        return sIsSupported;
    }

    /**
     * Checks if app banners are enabled for the tab which this manager is attached to.
     * @return true if app banners can be shown for this tab, false otherwise.
     */
    @CalledByNative
    private boolean isEnabledForTab() {
        return isSupported() && mIsEnabledForTab;
    }

    /**
     * Sets the delegate that provides information about a given package.
     * @param delegate Delegate to use.  Previously set ones are destroyed.
     */
    public static void setAppDetailsDelegate(AppDetailsDelegate delegate) {
        if (sAppDetailsDelegate != null) sAppDetailsDelegate.destroy();
        sAppDetailsDelegate = delegate;
    }

    /**
     * Constructs an AppBannerManager.
     * @param nativePointer the native-side object that owns this AppBannerManager.
     */
    private AppBannerManager(long nativePointer) {
        mNativePointer = nativePointer;
        mIsEnabledForTab = isSupported();
    }

    @CalledByNative
    private static AppBannerManager create(long nativePointer) {
        return new AppBannerManager(nativePointer);
    }

    @CalledByNative
    private void destroy() {
        mNativePointer = 0;
    }

    /**
     * Grabs package information for the banner asynchronously.
     * @param url         URL for the page that is triggering the banner.
     * @param packageName Name of the package that is being advertised.
     */
    @CalledByNative
    private void fetchAppDetails(
            String url, String packageName, String referrer, int iconSizeInDp) {
        if (sAppDetailsDelegate == null) return;

        Context context = ContextUtils.getApplicationContext();
        int iconSizeInPx = Math.round(
                context.getResources().getDisplayMetrics().density * iconSizeInDp);
        sAppDetailsDelegate.getAppDetailsAsynchronously(
                createAppDetailsObserver(), url, packageName, referrer, iconSizeInPx);
    }

    private AppDetailsDelegate.Observer createAppDetailsObserver() {
        return new AppDetailsDelegate.Observer() {
            /**
             * Called when data about the package has been retrieved, which includes the url for the
             * app's icon but not the icon Bitmap itself.
             * @param data Data about the app.  Null if the task failed.
             */
            @Override
            public void onAppDetailsRetrieved(AppData data) {
                if (data == null || mNativePointer == 0) return;

                String imageUrl = data.imageUrl();
                if (TextUtils.isEmpty(imageUrl)) return;

                nativeOnAppDetailsRetrieved(
                        mNativePointer, data, data.title(), data.packageName(), data.imageUrl());
            }
        };
    }

    /** Enables or disables app banners. */
    public void setIsEnabledForTab(boolean state) {
        mIsEnabledForTab = state;
    }

    /** Returns the language option to use for the add to homescreen dialog and menu item. */
    public static int getHomescreenLanguageOption() {
        int languageOption = nativeGetHomescreenLanguageOption();
        if (languageOption == LanguageOption.INSTALL) {
            return R.string.menu_add_to_homescreen_install;
        }
        return R.string.menu_add_to_homescreen;
    }

    /** Returns the language option to use for app banners. */
    public static int getAppBannerLanguageOption() {
        int languageOption = nativeGetHomescreenLanguageOption();
        if (languageOption == LanguageOption.ADD) {
            return R.string.app_banner_add;
        } else if (languageOption == LanguageOption.INSTALL) {
            return R.string.app_banner_install;
        }
        return R.string.menu_add_to_homescreen;
    }

    @VisibleForTesting
    public AddToHomescreenDialog getAddToHomescreenDialogForTesting() {
        return nativeGetAddToHomescreenDialogForTesting(mNativePointer);
    }

    /** Overrides whether the system supports add to home screen. Used in testing. */
    @VisibleForTesting
    public static void setIsSupported(boolean state) {
        sIsSupported = state;
    }

    /** Returns whether the native AppBannerManager is working. */
    @VisibleForTesting
    public boolean isRunningForTesting() {
        return nativeIsRunningForTesting(mNativePointer);
    }

    /** Signal to native that the add to homescreen menu item was tapped for metrics purposes. */
    public void recordMenuItemAddToHomescreen() {
        nativeRecordMenuItemAddToHomescreen(mNativePointer);
    }

    /** Signal to native that the menu was opened for metrics purposes. */
    public void recordMenuOpen() {
        nativeRecordMenuOpen(mNativePointer);
    }

    /** Sets constants (in days) the banner should be blocked for after dismissing and ignoring. */
    @VisibleForTesting
    static void setDaysAfterDismissAndIgnoreForTesting(int dismissDays, int ignoreDays) {
        nativeSetDaysAfterDismissAndIgnoreToTrigger(dismissDays, ignoreDays);
    }

    /** Sets a constant (in days) that gets added to the time when the current time is requested. */
    @VisibleForTesting
    static void setTimeDeltaForTesting(int days) {
        nativeSetTimeDeltaForTesting(days);
    }

    /** Sets the total required engagement to trigger the banner. */
    @VisibleForTesting
    static void setTotalEngagementForTesting(double engagement) {
        nativeSetTotalEngagementToTrigger(engagement);
    }

    /** Returns the AppBannerManager object. This is owned by the C++ banner manager. */
    public static AppBannerManager getAppBannerManagerForWebContents(WebContents webContents) {
        return nativeGetJavaBannerManagerForWebContents(webContents);
    }

    private static native int nativeGetHomescreenLanguageOption();
    private native void nativeRecordMenuItemAddToHomescreen(long nativeAppBannerManagerAndroid);
    private native void nativeRecordMenuOpen(long nativeAppBannerManagerAndroid);
    private static native AppBannerManager nativeGetJavaBannerManagerForWebContents(
            WebContents webContents);
    private native boolean nativeOnAppDetailsRetrieved(long nativeAppBannerManagerAndroid,
            AppData data, String title, String packageName, String imageUrl);

    // Testing methods.
    private native AddToHomescreenDialog nativeGetAddToHomescreenDialogForTesting(
            long nativeAppBannerManagerAndroid);
    private native boolean nativeIsRunningForTesting(long nativeAppBannerManagerAndroid);
    private static native void nativeSetDaysAfterDismissAndIgnoreToTrigger(
            int dismissDays, int ignoreDays);
    private static native void nativeSetTimeDeltaForTesting(int days);
    private static native void nativeSetTotalEngagementToTrigger(double engagement);
}
