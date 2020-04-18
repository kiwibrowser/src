// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.superviseduser;

import android.annotation.TargetApi;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemClock;
import android.os.UserManager;
import android.text.TextUtils;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.childaccounts.ChildAccountService;
import org.chromium.chrome.browser.firstrun.ForcedSigninProcessor;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.components.supervisedusererrorpage.FilteringBehaviorReason;
import org.chromium.components.webrestrictions.browser.WebRestrictionsContentProvider;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Content provider for telling other apps (e.g. WebView apps) about the supervised user URL filter.
 */
public class SupervisedUserContentProvider extends WebRestrictionsContentProvider {
    private static final String SUPERVISED_USER_CONTENT_PROVIDER_ENABLED =
            "SupervisedUserContentProviderEnabled";
    private long mNativeSupervisedUserContentProvider;
    private boolean mChromeAlreadyStarted;
    private static Object sEnabledLock = new Object();
    private static Object sContentProviderLock = new Object();

    private static final String ACCOUNTS_GOOGLE_COM = "accounts.google.com";
    private static final String GOOGLE_PLAY_SERVICES_PACKAGE = "com.google.android.gms";

    // Three value "boolean" caching enabled state, null if not yet known.
    private static Boolean sEnabled;

    @VisibleForTesting
    void startForcedSigninProcessor(Context appContext, Runnable onComplete) {
        ForcedSigninProcessor.start(appContext, onComplete);
    }

    @VisibleForTesting
    void listenForChildAccountStatusChange(Callback<Boolean> callback) {
        ChildAccountService.listenForStatusChange(callback);
    }

    private long getSupervisedUserContentProvider() {
        // This may lock for some time, but is always called on a background thread, and will only
        // take significant time if the Chrome process isn't already running.
        synchronized (sContentProviderLock) {
            mChromeAlreadyStarted = LibraryLoader.isInitialized();
            if (mNativeSupervisedUserContentProvider != 0) {
                return mNativeSupervisedUserContentProvider;
            }
            final Context appContext = getContext().getApplicationContext();
            final SupervisedUserReply<Long> reply = new SupervisedUserReply<>();
            ThreadUtils.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        ChromeBrowserInitializer.getInstance(appContext).handleSynchronousStartup();
                    } catch (ProcessInitException e) {
                        reply.onQueryFinished(0L);
                        return;
                    }
                    final ChromeSigninController chromeSigninController =
                            ChromeSigninController.get();
                    if (chromeSigninController.isSignedIn()) {
                        reply.onQueryFinished(nativeCreateSupervisedUserContentProvider());
                        return;
                    }
                    // Try to sign in, Chrome needs to be signed in to get the URL filter.
                    startForcedSigninProcessor(appContext, new Runnable() {
                        @Override
                        public void run() {
                            if (!chromeSigninController.isSignedIn()) {
                                reply.onQueryFinished(0L);
                                return;
                            }
                            // Wait for the status change; Chrome can't check any URLs until this
                            // has happened.
                            listenForChildAccountStatusChange(new Callback<Boolean>() {
                                @Override
                                public void onResult(Boolean result) {
                                    reply.onQueryFinished(
                                            nativeCreateSupervisedUserContentProvider());
                                }
                            });
                        }
                    });
                }
            });
            try {
                Long result = reply.getResult();
                if (result == null) return 0;
                mNativeSupervisedUserContentProvider = result;
                return mNativeSupervisedUserContentProvider;
            } catch (InterruptedException e) {
                return 0;
            }
        }
    }

    static class SupervisedUserReply<T> {
        private static final long RESULT_TIMEOUT_SECONDS = 10;
        final BlockingQueue<T> mQueue = new ArrayBlockingQueue<>(1);

        void onQueryFinished(T reply) {
            // This must be called precisely once per query.
            mQueue.add(reply);
        }

        T getResult() throws InterruptedException {
            return mQueue.poll(RESULT_TIMEOUT_SECONDS, TimeUnit.SECONDS);
        }
    }

    static class SupervisedUserQueryReply extends SupervisedUserReply<WebRestrictionsResult> {
        // One of the following three functions must be called precisely once per query.

        @CalledByNative("SupervisedUserQueryReply")
        void onQueryComplete() {
            onQueryFinished(new WebRestrictionsResult(true, null, null));
        }

        @CalledByNative("SupervisedUserQueryReply")
        void onQueryFailed(int reason, int allowAccessRequests, int isChildAccount,
                String profileImageUrl, String profileImageUrl2, String custodian,
                String custodianEmail, String secondCustodian, String secondCustodianEmail) {
            int errorInt[] = new int[] {reason, allowAccessRequests, isChildAccount};
            String errorString[] = new String[] {
                    profileImageUrl,
                    profileImageUrl2,
                    custodian,
                    custodianEmail,
                    secondCustodian,
                    secondCustodianEmail
            };
            onQueryFinished(new WebRestrictionsResult(false, errorInt, errorString));
        }

        void onQueryFailedNoErrorData() {
            onQueryFinished(new WebRestrictionsResult(false, null, null));
        }
    }

    @Override
    protected WebRestrictionsResult shouldProceed(String callingPackage, final String url) {
        // This will be called on multiple threads (but never the UI thread),
        // see http://developer.android.com/guide/components/processes-and-threads.html#ThreadSafe.
        // The reply comes back on a different thread (possibly the UI thread) some time later.
        // As such it needs to correctly match the replies to the calls. It does this by creating a
        // reply object for each query, and passing this through the callback structure. The reply
        // object also handles waiting for the reply.
        long startTimeMs = SystemClock.elapsedRealtime();
        if (requestIsWhitelisted(callingPackage, url)) {
            return new WebRestrictionsResult(true, null, null);
        }

        final long contentProvider = getSupervisedUserContentProvider();
        if (contentProvider == 0) {
            return new WebRestrictionsResult(
                    false, new int[] {FilteringBehaviorReason.NOT_SIGNED_IN}, null);
        }

        final SupervisedUserQueryReply queryReply = new SupervisedUserQueryReply();
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                nativeShouldProceed(contentProvider, queryReply, url);
            }
        });
        try {
            // This will block until an onQueryComplete call on a different thread adds
            // something to the queue.
            WebRestrictionsResult result = queryReply.getResult();
            String histogramName = mChromeAlreadyStarted
                    ? "SupervisedUserContentProvider.ChromeStartedRequestTime"
                    : "SupervisedUserContentProvider.ChromeNotStartedRequestTime";
            RecordHistogram.recordTimesHistogram(histogramName,
                    SystemClock.elapsedRealtime() - startTimeMs, TimeUnit.MILLISECONDS);
            RecordHistogram.recordBooleanHistogram(
                    "SupervisedUserContentProvider.RequestTimedOut", result == null);
            if (result == null) return new WebRestrictionsResult(false, null, null);
            return result;
        } catch (InterruptedException e) {
            return new WebRestrictionsResult(false, null, null);
        }
    }

    private boolean requestIsWhitelisted(String callingPackage, String url) {
        // Always allow Google Play Services to show the reauthentication page (which is necessary
        // to fix account issues).
        return TextUtils.equals(callingPackage, GOOGLE_PLAY_SERVICES_PACKAGE)
                && Uri.parse(url).getHost().equals(ACCOUNTS_GOOGLE_COM);
    }

    @Override
    protected boolean canInsert() {
        // Chrome always allows insertion requests.
        return true;
    }

    static class SupervisedUserInsertReply extends SupervisedUserReply<Boolean> {
        @CalledByNative("SupervisedUserInsertReply")
        void onInsertRequestSendComplete(boolean result) {
            onQueryFinished(result);
        }
    }

    @Override
    protected boolean requestInsert(final String url) {
        // This will be called on multiple threads (but never the UI thread),
        // see http://developer.android.com/guide/components/processes-and-threads.html#ThreadSafe.
        // The reply comes back on a different thread (possibly the UI thread) some time later.
        // As such it needs to correctly match the replies to the calls. It does this by creating a
        // reply object for each query, and passing this through the callback structure. The reply
        // object also handles waiting for the reply.
        final SupervisedUserInsertReply insertReply = new SupervisedUserInsertReply();
        final long contentProvider = getSupervisedUserContentProvider();
        if (contentProvider == 0) return false;
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                nativeRequestInsert(contentProvider, insertReply, url);
            }
        });
        try {
            Boolean result = insertReply.getResult();
            if (result == null) return false;
            return result;
        } catch (InterruptedException e) {
            return false;
        }
    }

    @CalledByNative
    void onSupervisedUserFilterUpdated() {
        onFilterChanged();
    }

    private static Boolean getEnabled() {
        synchronized (sEnabledLock) {
            return sEnabled;
        }
    }

    private static void setEnabled(boolean enabled) {
        synchronized (sEnabledLock) {
            sEnabled = enabled;
        }
    }

    @Override
    protected boolean contentProviderEnabled() {
        if (getEnabled() != null) return getEnabled();
        // There wasn't a fully functional App Restrictions system in Android (including the
        // broadcast intent for updates) until Lollipop.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return false;
        updateEnabledState();
        createEnabledBroadcastReceiver();
        return getEnabled();
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private void createEnabledBroadcastReceiver() {
        IntentFilter restrictionsFilter = new IntentFilter(
                Intent.ACTION_APPLICATION_RESTRICTIONS_CHANGED);
        BroadcastReceiver restrictionsReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                updateEnabledState();
            }
        };
        getContext().registerReceiver(restrictionsReceiver, restrictionsFilter);
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
    private void updateEnabledState() {
        // This method uses AppRestrictions directly, rather than using the Policy interface,
        // because it must be callable in contexts in which the native library hasn't been
        // loaded. It will always be called from a background thread (except possibly in tests)
        // so can get the App Restrictions synchronously.
        UserManager userManager = (UserManager) getContext().getSystemService(Context.USER_SERVICE);
        Bundle appRestrictions = userManager
                .getApplicationRestrictions(getContext().getPackageName());
        setEnabled(appRestrictions.getBoolean(SUPERVISED_USER_CONTENT_PROVIDER_ENABLED));
    }

    @Override
    protected String[] getErrorColumnNames() {
        String result[] = {"Reason", "Allow access requests", "Is child account",
                "Profile image URL", "Second profile image URL", "Custodian", "Custodian email",
                "Second custodian", "Second custodian email"};
        return result;
    }

    // Helpers for testing.

    @Override
    public Bundle call(String method, String arg, Bundle bundle) {
        if (method.equals("setFilterForTesting")) setFilterForTesting();
        return null;
    }

    void setFilterForTesting() {
        final long contentProvider = getSupervisedUserContentProvider();
        if (contentProvider == 0) return;
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                nativeSetFilterForTesting(contentProvider);
            }
        });
    }

    void setNativeSupervisedUserContentProviderForTesting(long nativeProvider) {
        mNativeSupervisedUserContentProvider = nativeProvider;
    }

    @VisibleForTesting
    public static void enableContentProviderForTesting() {
        setEnabled(true);
    }

    native long nativeCreateSupervisedUserContentProvider();

    native void nativeShouldProceed(long nativeSupervisedUserContentProvider,
            SupervisedUserQueryReply queryReply, String url);

    native void nativeRequestInsert(long nativeSupervisedUserContentProvider,
            SupervisedUserInsertReply insertReply, String url);

    private native void nativeSetFilterForTesting(long nativeSupervisedUserContentProvider);

}
