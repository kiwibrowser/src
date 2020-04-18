// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.annotation.SuppressLint;
import android.content.Context;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.SystemClock;
import android.view.ContextThemeWrapper;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.FrameLayout;

import org.chromium.base.Log;
import org.chromium.base.StrictModeContext;
import org.chromium.base.SysUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.widget.ControlContainer;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;

import java.net.InetAddress;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * This class is a singleton that holds utilities for warming up Chrome and prerendering urls
 * without creating the Activity.
 *
 * This class is not thread-safe and must only be used on the UI thread.
 */
public final class WarmupManager {
    private static final String TAG = "WarmupManager";

    @VisibleForTesting
    static final String WEBCONTENTS_STATUS_HISTOGRAM = "CustomTabs.SpareWebContents.Status";

    // See CustomTabs.SpareWebContentsStatus histogram. Append-only.
    @VisibleForTesting
    static final int WEBCONTENTS_STATUS_CREATED = 0;
    @VisibleForTesting
    static final int WEBCONTENTS_STATUS_USED = 1;
    @VisibleForTesting
    static final int WEBCONTENTS_STATUS_KILLED = 2;
    @VisibleForTesting
    static final int WEBCONTENTS_STATUS_DESTROYED = 3;
    private static final int WEBCONTENTS_STATUS_COUNT = 4;

    /**
     * Observes spare WebContents deaths. In case of death, records stats, and cleanup the objects.
     */
    private class RenderProcessGoneObserver extends WebContentsObserver {
        @Override
        public void renderProcessGone(boolean wasOomProtected) {
            long elapsed = SystemClock.elapsedRealtime() - mWebContentsCreationTimeMs;
            RecordHistogram.recordLongTimesHistogram(
                    "CustomTabs.SpareWebContents.TimeBeforeDeath", elapsed, TimeUnit.MILLISECONDS);
            recordWebContentsStatus(WEBCONTENTS_STATUS_KILLED);
            destroySpareWebContentsInternal();
        }
    };

    @SuppressLint("StaticFieldLeak")
    private static WarmupManager sWarmupManager;

    private final Set<String> mDnsRequestsInFlight;
    private final Map<String, Profile> mPendingPreconnectWithProfile;

    private int mToolbarContainerId;
    private ViewGroup mMainView;
    @VisibleForTesting
    WebContents mSpareWebContents;
    private long mWebContentsCreationTimeMs;
    private RenderProcessGoneObserver mObserver;

    /**
     * @return The singleton instance for the WarmupManager, creating one if necessary.
     */
    public static WarmupManager getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sWarmupManager == null) sWarmupManager = new WarmupManager();
        return sWarmupManager;
    }

    private WarmupManager() {
        mDnsRequestsInFlight = new HashSet<>();
        mPendingPreconnectWithProfile = new HashMap<>();
    }

    /**
     * Inflates and constructs the view hierarchy that the app will use.
     * @param baseContext The base context to use for creating the ContextWrapper.
     * @param toolbarContainerId Id of the toolbar container.
     * @param toolbarId The toolbar's layout ID.
     */
    public void initializeViewHierarchy(Context baseContext, int toolbarContainerId,
            int toolbarId) {
        // Inflating the view hierarchy causes StrictMode violations on some
        // devices. Since layout inflation should happen on the UI thread, allow
        // the disk reads. crbug.com/644243.
        try (TraceEvent e = TraceEvent.scoped("WarmupManager.initializeViewHierarchy");
                StrictModeContext c = StrictModeContext.allowDiskReads()) {
            ThreadUtils.assertOnUiThread();
            if (mMainView != null && mToolbarContainerId == toolbarContainerId) return;
            ContextThemeWrapper context =
                    new ContextThemeWrapper(baseContext, ChromeActivity.getThemeId());
            FrameLayout contentHolder = new FrameLayout(context);
            mMainView = (ViewGroup) LayoutInflater.from(context).inflate(
                    R.layout.main, contentHolder);
            mToolbarContainerId = toolbarContainerId;
            if (toolbarContainerId != ChromeActivity.NO_CONTROL_CONTAINER) {
                ViewStub stub = (ViewStub) mMainView.findViewById(R.id.control_container_stub);
                stub.setLayoutResource(toolbarContainerId);
                ControlContainer controlContainer = (ControlContainer) stub.inflate();
                controlContainer.initWithToolbar(toolbarId);
            }
        } catch (InflateException e) {
            // See crbug.com/606715.
            Log.e(TAG, "Inflation exception.", e);
            mMainView = null;
        }
    }

    /**
     * Transfers all the children in the view hierarchy to the giving ViewGroup as child.
     * @param contentView The parent ViewGroup to use for the transfer.
     */
    public void transferViewHierarchyTo(ViewGroup contentView) {
        ThreadUtils.assertOnUiThread();
        ViewGroup viewHierarchy = mMainView;
        mMainView = null;
        if (viewHierarchy == null) return;
        while (viewHierarchy.getChildCount() > 0) {
            View currentChild = viewHierarchy.getChildAt(0);
            viewHierarchy.removeView(currentChild);
            contentView.addView(currentChild);
        }
    }

    /**
     * @return Whether a pre-built view hierarchy exists for the given toolbarContainerId.
     */
    public boolean hasViewHierarchyWithToolbar(int toolbarContainerId) {
        ThreadUtils.assertOnUiThread();
        return mMainView != null && mToolbarContainerId == toolbarContainerId;
    }

    /**
     * Clears the inflated view hierarchy.
     */
    public void clearViewHierarchy() {
        ThreadUtils.assertOnUiThread();
        mMainView = null;
    }

    /**
     * Launches a background DNS query for a given URL.
     *
     * @param url URL from which the domain to query is extracted.
     */
    private void prefetchDnsForUrlInBackground(final String url) {
        mDnsRequestsInFlight.add(url);
        new AsyncTask<String, Void, Void>() {
            @Override
            protected Void doInBackground(String... params) {
                try (TraceEvent e =
                                TraceEvent.scoped("WarmupManager.prefetchDnsForUrlInBackground")) {
                    InetAddress.getByName(new URL(url).getHost());
                } catch (MalformedURLException e) {
                    // We don't do anything with the result of the request, it
                    // is only here to warm up the cache, thus ignoring the
                    // exception is fine.
                } catch (UnknownHostException e) {
                    // As above.
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                mDnsRequestsInFlight.remove(url);
                if (mPendingPreconnectWithProfile.containsKey(url)) {
                    Profile profile = mPendingPreconnectWithProfile.get(url);
                    mPendingPreconnectWithProfile.remove(url);
                    maybePreconnectUrlAndSubResources(profile, url);
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, url);
    }

    /** Launches a background DNS query for a given URL if the data reduction proxy is not in use.
     *
     * @param context The Application context.
     * @param url URL from which the domain to query is extracted.
     */
    public void maybePrefetchDnsForUrlInBackground(Context context, String url) {
        ThreadUtils.assertOnUiThread();
        if (!DataReductionProxySettings.isEnabledBeforeNativeLoad(context)) {
            prefetchDnsForUrlInBackground(url);
        }
    }

    /**
     * Starts asynchronous initialization of the preconnect predictor.
     *
     * Without this call, |maybePreconnectUrlAndSubresources()| will not use a database of origins
     * to connect to, unless the predictor has already been initialized in another way.
     *
     * @param profile The profile to use for the predictor.
     */
    public static void startPreconnectPredictorInitialization(Profile profile) {
        ThreadUtils.assertOnUiThread();
        nativeStartPreconnectPredictorInitialization(profile);
    }

    /** Asynchronously preconnects to a given URL if the data reduction proxy is not in use.
     *
     * @param profile The profile to use for the preconnection.
     * @param url The URL we want to preconnect to.
     */
    public void maybePreconnectUrlAndSubResources(Profile profile, String url) {
        ThreadUtils.assertOnUiThread();

        Uri uri = Uri.parse(url);
        if (uri == null) return;
        String scheme = uri.normalizeScheme().getScheme();
        boolean isHttp = UrlConstants.HTTP_SCHEME.equals(scheme);
        if (!isHttp && !UrlConstants.HTTPS_SCHEME.equals(scheme)) return;
        // HTTP connections will not be used when the data reduction proxy is enabled.
        if (DataReductionProxySettings.getInstance().isDataReductionProxyEnabled() && isHttp) {
            return;
        }

        // If there is already a DNS request in flight for this URL, then the preconnection will
        // start by issuing a DNS request for the same domain, as the result is not cached. However,
        // such a DNS request has already been sent from this class, so it is better to wait for the
        // answer to come back before preconnecting. Otherwise, the preconnection logic will wait
        // for the result of the second DNS request, which should arrive after the result of the
        // first one. Note that we however need to wait for the main thread to be available in this
        // case, since the preconnection will be sent from AsyncTask.onPostExecute(), which may
        // delay it.
        if (mDnsRequestsInFlight.contains(url)) {
            // Note that if two requests come for the same URL with two different profiles, the last
            // one will win.
            mPendingPreconnectWithProfile.put(url, profile);
        } else {
            nativePreconnectUrlAndSubresources(profile, url);
        }
    }

    /**
     * Warms up a spare, empty RenderProcessHost that may be used for subsequent navigations.
     *
     * The spare RenderProcessHost will be used automatically in subsequent navigations.
     * There is nothing further the WarmupManager needs to do to enable that use.
     *
     * This uses a different mechanism than createSpareWebContents, below, and is subject
     * to fewer restrictions.
     *
     * This must be called from the UI thread.
     */
    public void createSpareRenderProcessHost(Profile profile) {
        ThreadUtils.assertOnUiThread();
        if (!LibraryLoader.isInitialized()) return;
        if (ChromeFeatureList.isEnabled(ChromeFeatureList.OMNIBOX_SPARE_RENDERER)) {
            // Spare WebContents should not be used with spare RenderProcessHosts, but if one
            // has been created, destroy it in order not to consume too many processes.
            destroySpareWebContents();
            nativeWarmupSpareRenderer(profile);
        }
    }

    /**
     * Creates and initializes a spare WebContents, to be used in a subsequent navigation.
     *
     * This creates a renderer that is suitable for any navigation. It can be picked up by any tab.
     * Can be called multiple times, and must be called from the UI thread.
     * Note that this is a no-op on low-end devices.
     */
    public void createSpareWebContents() {
        ThreadUtils.assertOnUiThread();
        if (!LibraryLoader.isInitialized()) return;
        if (mSpareWebContents != null || SysUtils.isLowEndDevice()) return;
        mSpareWebContents = WebContentsFactory.createWebContentsWithWarmRenderer(
                false /* incognito */, true /* initiallyHidden */);
        mObserver = new RenderProcessGoneObserver();
        mSpareWebContents.addObserver(mObserver);
        mWebContentsCreationTimeMs = SystemClock.elapsedRealtime();
        recordWebContentsStatus(WEBCONTENTS_STATUS_CREATED);
    }

    /**
     * Destroys the spare WebContents if there is one.
     */
    public void destroySpareWebContents() {
        ThreadUtils.assertOnUiThread();
        if (mSpareWebContents == null) return;
        recordWebContentsStatus(WEBCONTENTS_STATUS_DESTROYED);
        destroySpareWebContentsInternal();
    }

    /**
     * Returns a spare WebContents or null, depending on the availability of one.
     *
     * The parameters are the same as for {@link WebContentsFactory#createWebContents()}.
     *
     * @return a WebContents, or null.
     */
    public WebContents takeSpareWebContents(boolean incognito, boolean initiallyHidden) {
        ThreadUtils.assertOnUiThread();
        if (incognito) return null;
        WebContents result = mSpareWebContents;
        if (result == null) return null;
        mSpareWebContents = null;
        result.removeObserver(mObserver);
        mObserver = null;
        if (!initiallyHidden) result.onShow();
        recordWebContentsStatus(WEBCONTENTS_STATUS_USED);
        return result;
    }

    /**
     * @return Whether a spare renderer is available.
     */
    public boolean hasSpareWebContents() {
        return mSpareWebContents != null;
    }

    private void destroySpareWebContentsInternal() {
        mSpareWebContents.removeObserver(mObserver);
        mSpareWebContents.destroy();
        mSpareWebContents = null;
        mObserver = null;
    }

    private static void recordWebContentsStatus(int status) {
        RecordHistogram.recordEnumeratedHistogram(
                WEBCONTENTS_STATUS_HISTOGRAM, status, WEBCONTENTS_STATUS_COUNT);
    }

    private static native void nativeStartPreconnectPredictorInitialization(Profile profile);
    private static native void nativePreconnectUrlAndSubresources(Profile profile, String url);
    private static native void nativeWarmupSpareRenderer(Profile profile);
}
