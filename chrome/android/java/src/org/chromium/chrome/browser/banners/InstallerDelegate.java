// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.banners;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.support.annotation.IntDef;
import android.text.TextUtils;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ui.base.WindowAndroid;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.concurrent.TimeUnit;

/**
 * Monitors the app installation process and informs an observer of updates.
 */
public class InstallerDelegate {
    /** Observer methods called for the different stages of app installation. */
    public static interface Observer {
        /**
         * Called when the installation intent completes to inform the observer if installation has
         * begun. For instance, the user may have rejected the intent.
         * @param delegate     The delegate object sending the message.
         * @param isInstalling true if the app is currently installing.
         */
        public void onInstallIntentCompleted(InstallerDelegate delegate, boolean isInstalling);

        /**
         * Called when the task has finished.
         * @param delegate The delegate object sending the message.
         * @param success  Whether or not the package was successfully installed.
         */
        public void onInstallFinished(InstallerDelegate delegate, boolean success);

        /**
         * Called when the current application state changes due to an installation.
         * @param delegate The delegate object sending the message.
         * @param newState The new state id.
         */
        public void onApplicationStateChanged(
                InstallerDelegate delegate, @ActivityState int newState);
    }

    /**
     * Object to wait for the PackageManager to finish installation an app. For convenience, this is
     * bound to an instance of InstallerDelegate, and accesses its members and methods.
     */
    private class InstallMonitor implements Runnable {
        /** Timestamp of when we started monitoring. */
        private long mTimestampStarted;

        public InstallMonitor() {}

        /** Begin monitoring the PackageManager to see if it completes installing the package. */
        public void start() {
            mTimestampStarted = SystemClock.elapsedRealtime();
            mIsRunning = true;
            mHandler.postDelayed(this, mMsBetweenRuns);
        }

        /** Don't call this directly; instead, call {@link #start()}. */
        @Override
        public void run() {
            boolean isPackageInstalled = isInstalled(mPackageName);
            boolean waitedTooLong =
                    (SystemClock.elapsedRealtime() - mTimestampStarted) > mMsMaximumWaitingTime;
            if (isPackageInstalled || !mIsRunning || waitedTooLong) {
                mIsRunning = false;
                mObserver.onInstallFinished(InstallerDelegate.this, isPackageInstalled);
            } else {
                mHandler.postDelayed(this, mMsBetweenRuns);
            }
        }

        /** Prevent rescheduling the Runnable. */
        public void cancel() {
            mIsRunning = false;
        }
    }

    // Installation states.
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({INSTALL_STATE_NOT_INSTALLED, INSTALL_STATE_INSTALLING, INSTALL_STATE_INSTALLED})
    public @interface InstallState {}
    public static final int INSTALL_STATE_NOT_INSTALLED = 0;
    public static final int INSTALL_STATE_INSTALLING = 1;
    public static final int INSTALL_STATE_INSTALLED = 2;

    private static final String TAG = "cr_InstallerDelegate";
    private static final long DEFAULT_MS_BETWEEN_RUNS = TimeUnit.SECONDS.toMillis(1);
    private static final long DEFAULT_MS_MAXIMUM_WAITING_TIME = TimeUnit.MINUTES.toMillis(3);

    /** PackageManager to use in place of the real one. */
    private static PackageManager sPackageManagerForTests;

    /** Monitors an installation in progress. */
    private InstallMonitor mInstallMonitor;

    /** Message loop to post the Runnable to. */
    private final Handler mHandler;

    /** Monitors for application state changes. */
    private final ApplicationStatus.ApplicationStateListener mListener;

    /** The name of the package currently being installed. */
    private String mPackageName;

    /** Milliseconds to wait between calls to check on the PackageManager during installation. */
    private long mMsBetweenRuns;

    /** Maximum milliseconds to wait before giving up on monitoring during installation. */
    private long mMsMaximumWaitingTime;

    /** The observer to inform of updates during the installation process. */
    private Observer mObserver;

    /** Whether or we are currently monitoring an installation. */
    private boolean mIsRunning;

    /** Overrides the PackageManager for testing. */
    @VisibleForTesting
    static void setPackageManagerForTesting(PackageManager manager) {
        sPackageManagerForTests = manager;
    }

    /**
     * Set how often the handler will check the PackageManager.
     * @param msBetween How long to wait between executions of the Runnable.
     * @param msMax     How long to wait before giving up.
     */
    @VisibleForTesting
    void setTimingForTests(long msBetween, long msMax) {
        mMsBetweenRuns = msBetween;
        mMsMaximumWaitingTime = msMax;
    }

    /**
     * Checks if the app has been installed on the system.
     * @return true if the PackageManager reports that the app is installed, false otherwise.
     * @param packageManager PackageManager to use.
     * @param packageName    Name of the package to check.
     */
    public static boolean isInstalled(PackageManager packageManager, String packageName) {
        try {
            packageManager.getPackageInfo(packageName, 0);
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
        return true;
    }

    /**
     * Construct an InstallerDelegate to monitor an installation.
     * @param looper   Thread to run the monitor on.
     * @param observer Object to inform of changes in the installation state.
     */
    public InstallerDelegate(Looper looper, Observer observer) {
        mHandler = new Handler(looper);
        mObserver = observer;
        mListener = createApplicationStateListener();
        ApplicationStatus.registerApplicationStateListener(mListener);

        mMsBetweenRuns = DEFAULT_MS_BETWEEN_RUNS;
        mMsMaximumWaitingTime = DEFAULT_MS_MAXIMUM_WAITING_TIME;
    }

    /**
     * Checks if the app has been installed on the system.
     * @return true if the PackageManager reports that the app is installed, false otherwise.
     * @param packageName Name of the package to check.
     */
    public boolean isInstalled(String packageName) {
        return isInstalled(getPackageManager(ContextUtils.getApplicationContext()), packageName);
    }

    /**
     * Stops all current monitoring.
     */
    public void destroy() {
        if (mInstallMonitor != null) {
            mInstallMonitor.cancel();
            mInstallMonitor = null;
        }
        ApplicationStatus.unregisterApplicationStateListener(mListener);
    }

    /**
     * Returns the current installation state of the provided package name
     * @return the installation state - not installed, installing, or installed.
     * @param packageName Name of the package to check.
     */
    public @InstallState int determineInstallState(String packageName) {
        if (mIsRunning) {
            return INSTALL_STATE_INSTALLING;
        }

        if (isInstalled(packageName)) {
            return INSTALL_STATE_INSTALLED;
        }

        return INSTALL_STATE_NOT_INSTALLED;
    }

    /**
     * Attempts to open the specified package name in the given content.
     * @return true if successfully opened, otherwise false (e.g. not installed/opening failed).
     * @param packageName Name of the package to open.
     */
    public boolean openApp(String packageName) {
        Context context = ContextUtils.getApplicationContext();
        PackageManager packageManager = getPackageManager(context);
        if (!isInstalled(packageManager, packageName)) return false;

        Intent launchIntent = packageManager.getLaunchIntentForPackage(packageName);
        if (launchIntent != null) {
            try {
                context.startActivity(launchIntent);
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "Failed to open app : %s!", packageName, e);
                return false;
            }
        }
        return true;
    }

    /**
     * Attempts to install or open a native app specified by the given data.
     * @return true if the app was opened or installation was started successfully. false otherwise.
     * @param tab      The current tab.
     * @param appData  The native app data to try and open or install.
     * @param referrer The referrer attached to the URL specifying the native app, if any.
     */
    public boolean installOrOpenNativeApp(Tab tab, AppData appData, String referrer) {
        if (openApp(appData.packageName())) {
            return true;
        } else {
            // If the installation was started, return false to prevent the infobar disappearing.
            // The supplied referrer is the URL of the page requesting the native app banner. It may
            // be empty depending on that page's referrer policy. If it is non-empty, attach it to
            // the installation intent as Intent.EXTRA_REFERRER.
            Intent installIntent = appData.installIntent();
            if (!TextUtils.isEmpty(referrer)) {
                installIntent.putExtra(Intent.EXTRA_REFERRER, referrer);
            }
            return !tab.getWindowAndroid().showIntent(
                    installIntent, createIntentCallback(appData), null);
        }
    }

    /**
     * Start monitoring an installation. Should be called once per the lifetime of this object.
     * @param packageName The name of the package to monitor.
     * */
    public void startMonitoring(String packageName) {
        mPackageName = packageName;
        mInstallMonitor = new InstallMonitor();
        mInstallMonitor.start();
    }

    /** Checks to see if we are currently monitoring an installation. */
    @VisibleForTesting
    boolean isRunning() {
        return mIsRunning;
    }

    /** Simulates a cancellation for testing purposes. */
    @VisibleForTesting
    void cancel() {
        mInstallMonitor.cancel();
    }

    private WindowAndroid.IntentCallback createIntentCallback(final AppData appData) {
        return new WindowAndroid.IntentCallback() {
            @Override
            public void onIntentCompleted(WindowAndroid window, int resultCode, Intent data) {
                boolean isInstalling = resultCode == Activity.RESULT_OK;
                if (isInstalling) {
                    startMonitoring(appData.packageName());
                }

                mObserver.onInstallIntentCompleted(InstallerDelegate.this, isInstalling);
            }
        };
    }

    private ApplicationStatus.ApplicationStateListener createApplicationStateListener() {
        return new ApplicationStatus.ApplicationStateListener() {
            @Override
            public void onApplicationStateChange(int newState) {
                if (!ApplicationStatus.hasVisibleActivities()) return;
                mObserver.onApplicationStateChanged(InstallerDelegate.this, newState);
            }
        };
    }

    private PackageManager getPackageManager(Context context) {
        if (sPackageManagerForTests != null) return sPackageManagerForTests;
        return context.getPackageManager();
    }
}
