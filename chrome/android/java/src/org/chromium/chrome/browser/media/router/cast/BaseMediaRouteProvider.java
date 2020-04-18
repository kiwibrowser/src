// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.media.router.cast;

import android.os.Handler;
import android.support.v7.media.MediaRouteSelector;
import android.support.v7.media.MediaRouter;
import android.support.v7.media.MediaRouter.RouteInfo;

import org.chromium.base.Log;
import org.chromium.chrome.browser.media.router.DiscoveryCallback;
import org.chromium.chrome.browser.media.router.DiscoveryDelegate;
import org.chromium.chrome.browser.media.router.MediaController;
import org.chromium.chrome.browser.media.router.MediaRoute;
import org.chromium.chrome.browser.media.router.MediaRouteManager;
import org.chromium.chrome.browser.media.router.MediaRouteProvider;
import org.chromium.chrome.browser.media.router.MediaSink;
import org.chromium.chrome.browser.media.router.MediaSource;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

/**
 * A {@link BaseMediaRouteProvider} common implementation for MediaRouteProviders.
 */
public abstract class BaseMediaRouteProvider
        implements MediaRouteProvider, DiscoveryDelegate,
                   ChromeCastSessionManager.CastSessionManagerListener {
    private static final String TAG = "MediaRouter";

    protected static final List<MediaSink> NO_SINKS = Collections.emptyList();

    protected final MediaRouter mAndroidMediaRouter;
    protected final MediaRouteManager mManager;
    protected final Map<String, DiscoveryCallback> mDiscoveryCallbacks =
            new HashMap<String, DiscoveryCallback>();
    protected final Map<String, MediaRoute> mRoutes = new HashMap<String, MediaRoute>();
    protected Handler mHandler = new Handler();

    // There can be only one Cast session at the same time on Android.
    protected CastSession mSession;

    protected BaseMediaRouteProvider(MediaRouter androidMediaRouter, MediaRouteManager manager) {
        mAndroidMediaRouter = androidMediaRouter;
        mManager = manager;
    }

    /**
     * @return A MediaSource object constructed from |sourceId|, or null if the derived class does
     * not support the source.
     */
    @Nullable
    protected abstract MediaSource getSourceFromId(@Nonnull String sourceId);

    /**
     * @return A CastSessionLaunchRequest encapsulating a session launch request.
     */
    @Nullable
    protected abstract ChromeCastSessionManager.CastSessionLaunchRequest createSessionLaunchRequest(
            MediaSource source, MediaSink sink, String presentationId, String origin, int tabId,
            boolean isIncognito, int nativeRequestId);

    /**
     * Forward the sinks back to the native counterpart.
     */
    // Migrated to CafMediaRouteProvider. See https://crbug.com/711860.
    protected void onSinksReceivedInternal(String sourceId, @Nonnull List<MediaSink> sinks) {
        Log.d(TAG, "Reporting %d sinks for source: %s", sinks.size(), sourceId);
        mManager.onSinksReceived(sourceId, this, sinks);
    }

    /**
     * {@link DiscoveryDelegate} implementation.
     */
    // Migrated to CafMediaRouteProvider. See https://crbug.com/711860.
    @Override
    public void onSinksReceived(String sourceId, @Nonnull List<MediaSink> sinks) {
        Log.d(TAG, "Received %d sinks for sourceId: %s", sinks.size(), sourceId);
        mHandler.post(() -> { onSinksReceivedInternal(sourceId, sinks); });
    }

    /**
     * {@link MediaRouteProvider} implementation.
     */
    // Migrated to CafMediaRouteProvider. See https://crbug.com/711860.
    @Override
    public boolean supportsSource(@Nonnull String sourceId) {
        return getSourceFromId(sourceId) != null;
    }

    // Migrated to CafMediaRouteProvider. See https://crbug.com/711860.
    @Override
    public void startObservingMediaSinks(@Nonnull String sourceId) {
        Log.d(TAG, "startObservingMediaSinks: " + sourceId);

        if (mAndroidMediaRouter == null) {
            // If the MediaRouter API is not available, report no devices so the page doesn't even
            // try to cast.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        MediaSource source = getSourceFromId(sourceId);
        if (source == null) {
            // If the source is invalid or not supported by this provider, report no devices
            // available.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        // No-op, if already monitoring the application for this source.
        String applicationId = source.getApplicationId();
        DiscoveryCallback callback = mDiscoveryCallbacks.get(applicationId);
        if (callback != null) {
            callback.addSourceUrn(sourceId);
            return;
        }

        MediaRouteSelector routeSelector = source.buildRouteSelector();
        if (routeSelector == null) {
            // If the application invalid, report no devices available.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        List<MediaSink> knownSinks = new ArrayList<MediaSink>();
        for (RouteInfo route : mAndroidMediaRouter.getRoutes()) {
            if (route.matchesSelector(routeSelector)) {
                knownSinks.add(MediaSink.fromRoute(route));
            }
        }

        callback = new DiscoveryCallback(sourceId, knownSinks, this, routeSelector);
        mAndroidMediaRouter.addCallback(
                routeSelector, callback, MediaRouter.CALLBACK_FLAG_REQUEST_DISCOVERY);
        mDiscoveryCallbacks.put(applicationId, callback);
    }

    // Migrated to CafMediaRouteProvider. See https://crbug.com/711860.
    @Override
    public void stopObservingMediaSinks(@Nonnull String sourceId) {
        Log.d(TAG, "stopObservingMediaSinks: " + sourceId);
        if (mAndroidMediaRouter == null) return;

        MediaSource source = getSourceFromId(sourceId);
        if (source == null) return;

        String applicationId = source.getApplicationId();
        DiscoveryCallback callback = mDiscoveryCallbacks.get(applicationId);
        if (callback == null) return;

        callback.removeSourceUrn(sourceId);

        if (callback.isEmpty()) {
            mAndroidMediaRouter.removeCallback(callback);
            mDiscoveryCallbacks.remove(applicationId);
        }
    }

    @Override
    public void createRoute(String sourceId, String sinkId, String presentationId, String origin,
            int tabId, boolean isIncognito, int nativeRequestId) {
        if (mAndroidMediaRouter == null) {
            mManager.onRouteRequestError("Not supported", nativeRequestId);
            return;
        }

        MediaSink sink = MediaSink.fromSinkId(sinkId, mAndroidMediaRouter);
        if (sink == null) {
            mManager.onRouteRequestError("No sink", nativeRequestId);
            return;
        }

        MediaSource source = getSourceFromId(sourceId);
        if (source == null) {
            mManager.onRouteRequestError("Unsupported source URL", nativeRequestId);
            return;
        }

        ChromeCastSessionManager.CastSessionLaunchRequest request = createSessionLaunchRequest(
                source, sink, presentationId, origin, tabId, isIncognito, nativeRequestId);

        ChromeCastSessionManager.get().requestSessionLaunch(request);
    }

    @Override
    public abstract void joinRoute(
            String sourceId, String presentationId, String origin, int tabId, int nativeRequestId);

    @Override
    public abstract void closeRoute(String routeId);

    @Override
    public abstract void detachRoute(String routeId);

    @Override
    public abstract void sendStringMessage(String routeId, String message, int nativeCallbackId);

    // ChromeCastSessionObserver implementation.
    @Override
    public abstract void onSessionStarting(
            ChromeCastSessionManager.CastSessionLaunchRequest originalRequest);

    @Override
    public abstract void onSessionEnded();

    @Override
    public void onSessionStartFailed() {
        for (String routeId : mRoutes.keySet()) {
            mManager.onRouteClosedWithError(routeId, "Launch error");
        }
        mRoutes.clear();
    };

    @Override
    public void onSessionStarted(CastSession session) {
        mSession = session;
    }

    @Override
    public void onSessionStopAction() {
        if (mSession == null) return;

        for (String routeId : mRoutes.keySet()) closeRoute(routeId);
    }

    @Override
    @Nullable
    public MediaController getMediaController(String routeId) {
        return null;
    }
}
