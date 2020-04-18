// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.sync;

import android.accounts.Account;
import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SyncStatusObserver;
import android.os.Bundle;
import android.os.StrictMode;

import org.chromium.base.Callback;
import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ChromeSigninController;

import javax.annotation.Nullable;
import javax.annotation.concurrent.ThreadSafe;

/**
 * A helper class to handle the current status of sync for Chrome in Android settings.
 *
 * It also provides an observer to be used whenever Android sync settings change.
 *
 * This class is a collection of static methods so that no references to its object can be
 * stored. This is important because tests need to be able to overwrite the object with a
 * mock content resolver and know that no references to the old one are cached.
 *
 * This class must be initialized via updateAccount() on startup if the user is signed in.
 */
@ThreadSafe
public class AndroidSyncSettings {
    public static final String TAG = "AndroidSyncSettings";

    /**
     * Lock for ensuring singleton instantiation across threads.
     */
    private static final Object CLASS_LOCK = new Object();

    @SuppressLint("StaticFieldLeak")
    private static AndroidSyncSettings sInstance;

    private final Object mLock = new Object();

    private final String mContractAuthority;

    private final Context mApplicationContext;

    private final SyncContentResolverDelegate mSyncContentResolverDelegate;

    private Account mAccount;

    private boolean mIsSyncable;

    private boolean mChromeSyncEnabled;

    private boolean mMasterSyncEnabled;

    private final ObserverList<AndroidSyncSettingsObserver> mObservers =
            new ObserverList<AndroidSyncSettingsObserver>();

    /**
     * Provides notifications when Android sync settings have changed.
     */
    public interface AndroidSyncSettingsObserver { public void androidSyncSettingsChanged(); }

    private static void ensureInitialized(Context context) {
        synchronized (CLASS_LOCK) {
            if (sInstance == null) {
                SyncContentResolverDelegate contentResolver =
                        new SystemSyncContentResolverDelegate();
                sInstance = new AndroidSyncSettings(context, contentResolver);
            }
        }
    }

    @VisibleForTesting
    public static void overrideForTests(Context context,
            SyncContentResolverDelegate contentResolver, @Nullable Callback<Boolean> callback) {
        synchronized (CLASS_LOCK) {
            sInstance = new AndroidSyncSettings(context, contentResolver, callback);
        }
    }

    /**
     * @param context the context the ApplicationContext will be retrieved from.
     * @param syncContentResolverDelegate an implementation of {@link SyncContentResolverDelegate}.
     */
    private AndroidSyncSettings(
            Context context, SyncContentResolverDelegate syncContentResolverDelegate) {
        this(context, syncContentResolverDelegate, null);
    }

    /**
     * @param context the context the ApplicationContext will be retrieved from.
     * @param syncContentResolverDelegate an implementation of {@link SyncContentResolverDelegate}.
     * @param callback Callback that will be called after updating account is finished. Boolean
     *                 passed to the callback indicates whether syncability was changed.
     */
    private AndroidSyncSettings(Context context,
            SyncContentResolverDelegate syncContentResolverDelegate,
            @Nullable Callback<Boolean> callback) {
        mApplicationContext = context.getApplicationContext();
        mContractAuthority = mApplicationContext.getPackageName();
        mSyncContentResolverDelegate = syncContentResolverDelegate;

        mAccount = ChromeSigninController.get().getSignedInUser();
        updateSyncability(callback);
        updateCachedSettings();

        mSyncContentResolverDelegate.addStatusChangeListener(
                ContentResolver.SYNC_OBSERVER_TYPE_SETTINGS,
                new AndroidSyncSettingsChangedObserver());
    }

    /**
     * Checks whether sync is currently enabled from Chrome for the currently signed in account.
     *
     * It checks both the master sync for the device, and Chrome sync setting for the given account.
     * If no user is currently signed in it returns false.
     *
     * @return true if sync is on, false otherwise
     */
    public static boolean isSyncEnabled(Context context) {
        ensureInitialized(context);
        return sInstance.mMasterSyncEnabled && sInstance.mChromeSyncEnabled;
    }

    /**
     * Checks whether sync is currently enabled from Chrome for a given account.
     *
     * It checks only Chrome sync setting for the given account,
     * and ignores the master sync setting.
     *
     * @return true if sync is on, false otherwise
     */
    @VisibleForTesting
    public static boolean isChromeSyncEnabled(Context context) {
        ensureInitialized(context);
        return sInstance.mChromeSyncEnabled;
    }

    /**
     * Checks whether the master sync flag for Android is currently enabled.
     */
    public static boolean isMasterSyncEnabled(Context context) {
        ensureInitialized(context);
        return sInstance.mMasterSyncEnabled;
    }

    /**
     * Make sure Chrome is syncable, and enable sync.
     */
    public static void enableChromeSync(Context context) {
        ensureInitialized(context);
        sInstance.setChromeSyncEnabled(true);
    }

    /**
     * Disables Android Chrome sync
     */
    public static void disableChromeSync(Context context) {
        ensureInitialized(context);
        sInstance.setChromeSyncEnabled(false);
    }

    /**
     * Must be called when a new account is signed in.
     */
    public static void updateAccount(Context context, Account account) {
        updateAccount(context, account, null);
    }

    /**
     * Must be called when a new account is signed in.
     * @param callback Callback that will be called after updating account is finished. Boolean
     *                 passed to the callback indicates whether syncability was changed.
     */
    @VisibleForTesting
    public static void updateAccount(
            Context context, Account account, @Nullable Callback<Boolean> callback) {
        ensureInitialized(context);
        synchronized (sInstance.mLock) {
            sInstance.mAccount = account;
            sInstance.updateSyncability(callback);
        }
        if (sInstance.updateCachedSettings()) {
            sInstance.notifyObservers();
        }
    }

    /**
     * Returns the contract authority to use when requesting sync.
     */
    public static String getContractAuthority(Context context) {
        ensureInitialized(context);
        return sInstance.mContractAuthority;
    }

    /**
     * Add a new AndroidSyncSettingsObserver.
     */
    public static void registerObserver(Context context, AndroidSyncSettingsObserver observer) {
        ensureInitialized(context);
        synchronized (sInstance.mLock) {
            sInstance.mObservers.addObserver(observer);
        }
    }

    /**
     * Remove an AndroidSyncSettingsObserver that was previously added.
     */
    public static void unregisterObserver(Context context, AndroidSyncSettingsObserver observer) {
        ensureInitialized(context);
        synchronized (sInstance.mLock) {
            sInstance.mObservers.removeObserver(observer);
        }
    }

    private void setChromeSyncEnabled(boolean value) {
        synchronized (mLock) {
            updateSyncability(null);
            if (value == mChromeSyncEnabled || mAccount == null) return;
            mChromeSyncEnabled = value;

            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
            mSyncContentResolverDelegate.setSyncAutomatically(mAccount, mContractAuthority, value);
            StrictMode.setThreadPolicy(oldPolicy);
        }
        notifyObservers();
    }

    /**
     * Ensure Chrome is registered with the Android Sync Manager iff signed in.
     *
     * This is what causes the "Chrome" option to appear in Settings -> Accounts -> Sync .
     * This function must be called within a synchronized block.
     */
    private void updateSyncability(@Nullable final Callback<Boolean> callback) {
        boolean shouldBeSyncable = mAccount != null;
        if (mIsSyncable == shouldBeSyncable) {
            if (callback != null) callback.onResult(false);
            return;
        }

        mIsSyncable = shouldBeSyncable;

        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
        // Make account syncable if there is one.
        if (shouldBeSyncable) {
            mSyncContentResolverDelegate.setIsSyncable(mAccount, mContractAuthority, 1);
            // This reduces unnecessary resource usage. See http://crbug.com/480688 for details.
            mSyncContentResolverDelegate.removePeriodicSync(
                    mAccount, mContractAuthority, Bundle.EMPTY);
        }
        StrictMode.setThreadPolicy(oldPolicy);

        // Disable the syncability of Chrome for all other accounts.
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                AccountManagerFacade.get().tryGetGoogleAccounts(new Callback<Account[]>() {
                    @Override
                    public void onResult(Account[] accounts) {
                        synchronized (mLock) {
                            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
                            for (Account account : accounts) {
                                if (!account.equals(mAccount)
                                        && mSyncContentResolverDelegate.getIsSyncable(
                                                   account, mContractAuthority)
                                                > 0) {
                                    mSyncContentResolverDelegate.setIsSyncable(
                                            account, mContractAuthority, 0);
                                }
                            }
                            StrictMode.setThreadPolicy(oldPolicy);
                        }

                        if (callback != null) callback.onResult(true);
                    }
                });
            }
        });
    }

    /**
     * Helper class to be used by observers whenever sync settings change.
     *
     * To register the observer, call AndroidSyncSettings.registerObserver(...).
     */
    private class AndroidSyncSettingsChangedObserver implements SyncStatusObserver {
        @Override
        public void onStatusChanged(int which) {
            if (which == ContentResolver.SYNC_OBSERVER_TYPE_SETTINGS) {
                // Sync settings have changed; update our cached values.
                if (updateCachedSettings()) {
                    // If something actually changed, tell our observers.
                    notifyObservers();
                }
            }
        }
    }

    /**
     * Update the three cached settings from the content resolver.
     *
     * @return Whether either chromeSyncEnabled or masterSyncEnabled changed.
     */
    private boolean updateCachedSettings() {
        synchronized (mLock) {
            boolean oldChromeSyncEnabled = mChromeSyncEnabled;
            boolean oldMasterSyncEnabled = mMasterSyncEnabled;

            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
            if (mAccount != null) {
                mIsSyncable =
                        mSyncContentResolverDelegate.getIsSyncable(mAccount, mContractAuthority)
                        == 1;
                mChromeSyncEnabled = mSyncContentResolverDelegate.getSyncAutomatically(
                        mAccount, mContractAuthority);
            } else {
                mIsSyncable = false;
                mChromeSyncEnabled = false;
            }
            mMasterSyncEnabled = mSyncContentResolverDelegate.getMasterSyncAutomatically();
            StrictMode.setThreadPolicy(oldPolicy);

            return oldChromeSyncEnabled != mChromeSyncEnabled
                    || oldMasterSyncEnabled != mMasterSyncEnabled;
        }
    }

    private void notifyObservers() {
        for (AndroidSyncSettingsObserver observer : mObservers) {
            observer.androidSyncSettingsChanged();
        }
    }
}
