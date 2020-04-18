// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.services;

import android.accounts.AccountManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.browser.ChromeApplication;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.init.EmptyBrowserParts;
import org.chromium.chrome.browser.signin.AccountTrackerService;
import org.chromium.chrome.browser.signin.SigninHelper;

/**
 * This receiver is notified when accounts are added, accounts are removed, or
 * an account's credentials (saved password, etc) are changed.
 * All public methods must be called from the UI thread.
 */
public class AccountsChangedReceiver extends BroadcastReceiver {
    private static final String TAG = "AccountsChangedRx";

    @Override
    public void onReceive(Context context, final Intent intent) {
        if (!AccountManager.LOGIN_ACCOUNTS_CHANGED_ACTION.equals(intent.getAction())) return;

        final Context appContext = context.getApplicationContext();
        AsyncTask<Void, Void, Void> task = new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                SigninHelper.updateAccountRenameData(appContext);
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                continueHandleAccountChangeIfNeeded(appContext);
            }
        };
        task.execute();
    }

    private void continueHandleAccountChangeIfNeeded(final Context context) {
        AccountTrackerService.get().invalidateAccountSeedStatus(
                false /* don't refresh right now */);
        boolean isChromeVisible = ApplicationStatus.hasVisibleActivities();
        if (isChromeVisible) {
            startBrowserIfNeededAndValidateAccounts(context);
        } else {
            // Notify SigninHelper of changed accounts (via shared prefs).
            SigninHelper.markAccountsChangedPref(context);
        }
    }

    private static void startBrowserIfNeededAndValidateAccounts(final Context context) {
        BrowserParts parts = new EmptyBrowserParts() {
            @Override
            public void finishNativeInitialization() {
                ThreadUtils.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        SigninHelper.get(context).validateAccountSettings(true);
                    }
                });
            }

            @Override
            public void onStartupFailure() {
                // Startup failed. So notify SigninHelper of changed accounts via
                // shared prefs.
                SigninHelper.markAccountsChangedPref(context);
            }
        };
        try {
            ChromeBrowserInitializer.getInstance(context).handlePreNativeStartup(parts);
            ChromeBrowserInitializer.getInstance(context).handlePostNativeStartup(true, parts);
        } catch (ProcessInitException e) {
            Log.e(TAG, "Unable to load native library.", e);
            ChromeApplication.reportStartupErrorAndExit(e);
        }
    }
}
