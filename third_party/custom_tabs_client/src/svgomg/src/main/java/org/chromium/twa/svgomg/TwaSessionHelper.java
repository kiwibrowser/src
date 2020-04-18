package org.chromium.twa.svgomg;
// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.support.customtabs.CustomTabsCallback;
import android.support.customtabs.CustomTabsClient;
import android.support.customtabs.CustomTabsIntent;
import android.support.customtabs.CustomTabsService;
import android.support.customtabs.CustomTabsServiceConnection;
import android.support.customtabs.CustomTabsSession;
import android.support.customtabs.TrustedWebUtils;
import android.util.Log;

import org.chromium.customtabsclient.shared.ServiceConnection;
import org.chromium.customtabsclient.shared.ServiceConnectionCallback;

import java.lang.ref.WeakReference;
import java.util.Arrays;
import java.util.List;

/**
 * When opening a TWA, it is necessary to connect to the CustomTabs Service, call warmup, create
 * a CustomTabsSession and validate the session against the TWA origin.
 *
 * This class helps to manage this lifecyle.
 *
 */
public class TwaSessionHelper implements ServiceConnectionCallback {
    private static final String TAG = TwaSessionHelper.class.getSimpleName();
    private static final List<String> CHROME_PACKAGES = Arrays.asList("com.chrome.canary");
    private static final TwaSessionHelper INSTANCE = new TwaSessionHelper();

    private CustomTabsSession mCustomTabsSession;
    private String packageName;
    private CustomTabsClient mClient;
    private CustomTabsServiceConnection mConnection;
    private Uri mOrigin;

    // This is a weak reference to avoid leaking the Activity.
    private WeakReference<TwaSessionCallback> mTwaSessionCallback;

    private TwaSessionHelper() {

    }

    public static TwaSessionHelper getInstance() {
        return INSTANCE;
    }

    /**
     * Opens the URL on a TWA.
     *
     * @param activity The host activity.
     * @param customTabsIntent a CustomTabsIntent to be used if Custom Tabs is available.
     * @param uri the Uri to be opened.
     */
    public void openTwa(Activity activity,
                                     CustomTabsIntent customTabsIntent,
                                     Uri uri) {

        // When opening a TWA, there are items on the Recents screen.
        // Workaround seems to be using the Intent.FLAG_ACTIVITY_NEW_DOCUMENT to create a new
        // document on Recents.
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.LOLLIPOP_MR1) {
            customTabsIntent.intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        } else {
            customTabsIntent.intent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        }

        // Ensure we're launching the same browser we're bound to.
        customTabsIntent.intent.setPackage(packageName);

        TrustedWebUtils.launchAsTrustedWebActivity(activity, customTabsIntent, uri);

        final TwaSessionCallback twaSessionCallback = mTwaSessionCallback.get();
        if (twaSessionCallback == null) {
            return;
        }

        // This should be called when the navigation callback for when the TWA is open
        // is received. The callbacks don't seem to be working at the moment, so using this
        // to improve the transition
        new Handler(activity.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                // Allow the host application to take action once the TWA is opened, such as
                // closing the launcher Activity.
                twaSessionCallback.onTwaOpened();
            }
        }, 1000L);
    }

    public CustomTabsIntent.Builder createIntentBuilder() {
        return new CustomTabsIntent.Builder(getSession());
    }

    /**
     * Unbinds the Activity from the Custom Tabs Service.
     * @param context the context that is connected to the service.
     */
    public void unbindService(Context context) {
        if (mConnection == null) return;
        Context applicationContext = context.getApplicationContext();
        applicationContext.unbindService(mConnection);
        mClient = null;
        mOrigin = null;
        mCustomTabsSession = null;
        mConnection = null;
    }

    /**
     * Creates or retrieves an exiting CustomTabsSession.
     *
     * @return a CustomTabsSession.
     */
    public CustomTabsSession getSession() {
        return getSession(new CustomTabsCallback());
    }

    /**
     * Creates or retrieves an exiting CustomTabsSession.
     *
     * @return a CustomTabsSession.
     */
    public CustomTabsSession getSession(CustomTabsCallback customTabsCallback) {
        if (mClient == null) {
            mCustomTabsSession = null;
        } else if (mCustomTabsSession == null) {
            Log.d(TAG, "Setting callback: " + customTabsCallback.getClass().getName());
            mCustomTabsSession = mClient.newSession(customTabsCallback);
        }
        return mCustomTabsSession;
    }

    /**
     * Register a Callback to be called when connected or disconnected from the Custom Tabs Service.
     * @param twaSessionCallback
     */
    public void setTwaSessionCallback(TwaSessionCallback twaSessionCallback) {
        this.mTwaSessionCallback = new WeakReference<>(twaSessionCallback);
    }

    /**
     * Binds the Activity to the Custom Tabs Service.
     * @param context the context to be binded to the service.
     * @param origin the origin for the website that will be opened in the TWA.
     */
    public void bindService(Context context, Uri origin) {
        if (mClient != null) {
            if (!origin.equals(mOrigin)) {
                throw new IllegalArgumentException(
                        "Trying to bind to a different origin. Should call unbindService first");
            }
            // If Service is already bound, just fire the callback.
            TwaSessionCallback twaSessionCallback = mTwaSessionCallback.get();
            if (twaSessionCallback != null) twaSessionCallback.onTwaSessionReady();
            return;
        }

        this.mOrigin = origin;
        this.packageName = CustomTabsClient.getPackageName(context, CHROME_PACKAGES, false);

        Context applicationContext = context.getApplicationContext();

        mConnection = new ServiceConnection(this);
        CustomTabsClient.bindCustomTabsService(applicationContext, packageName, mConnection);
    }

    @Override
    public void onServiceConnected(CustomTabsClient client) {
        mClient = client;
        mClient.warmup(0L);
        CustomTabsSession session = getSession();
        session.validateRelationship(
                CustomTabsService.RELATION_HANDLE_ALL_URLS, mOrigin, null);

        TwaSessionCallback twaSessionCallback = mTwaSessionCallback.get();
        if (twaSessionCallback != null) twaSessionCallback.onTwaSessionReady();
    }

    @Override
    public void onServiceDisconnected() {
        mClient = null;
        mCustomTabsSession = null;
        TwaSessionCallback twaSessionCallback = mTwaSessionCallback.get();
        if (twaSessionCallback != null) twaSessionCallback.onTwaSessionDestroyed();
    }

    /**
     * A Callback for when the service is connected or disconnected. Use those callbacks to
     * handle UI changes when the service is connected or disconnected.
     */
    public interface TwaSessionCallback {
        /**
         * Called when the service is connected.
         */
        void onTwaSessionReady();

        /**
         * Called when the service is disconnected.
         */
        void onTwaSessionDestroyed();

        /**
         * Called when the TWA has been opened.
         */
        void onTwaOpened();
    }

}
