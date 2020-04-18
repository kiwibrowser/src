// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.media.router.cast.remoting;

import android.support.v7.media.MediaRouter;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.media.router.ChromeMediaRouter;
import org.chromium.chrome.browser.media.router.MediaController;
import org.chromium.chrome.browser.media.router.MediaRoute;
import org.chromium.chrome.browser.media.router.MediaRouteManager;
import org.chromium.chrome.browser.media.router.MediaRouteProvider;
import org.chromium.chrome.browser.media.router.MediaSink;
import org.chromium.chrome.browser.media.router.MediaSource;
import org.chromium.chrome.browser.media.router.cast.BaseMediaRouteProvider;
import org.chromium.chrome.browser.media.router.cast.CastSession;
import org.chromium.chrome.browser.media.router.cast.ChromeCastSessionManager;
import org.chromium.chrome.browser.media.router.cast.CreateRouteRequest;

import javax.annotation.Nullable;

/**
 * A {@link MediaRouteProvider} implementation for media remote playback.
 */
public class RemotingMediaRouteProvider extends BaseMediaRouteProvider {
    private static final String TAG = "MediaRemoting";

    private int mPendingNativeRequestId;
    private MediaRoute mPendingMediaRoute;

    /**
     * @return Initialized {@link RemotingMediaRouteProvider} object.
     */
    public static RemotingMediaRouteProvider create(MediaRouteManager manager) {
        return new RemotingMediaRouteProvider(ChromeMediaRouter.getAndroidMediaRouter(), manager);
    }

    @Override
    protected MediaSource getSourceFromId(String sourceId) {
        return RemotingMediaSource.from(sourceId);
    }

    @Override
    protected ChromeCastSessionManager.CastSessionLaunchRequest createSessionLaunchRequest(
            MediaSource source, MediaSink sink, String presentationId, String origin, int tabId,
            boolean isIncognito, int nativeRequestId) {
        return new CreateRouteRequest(source, sink, presentationId, origin, tabId, isIncognito,
                nativeRequestId, this, CreateRouteRequest.RequestedCastSessionType.REMOTE, null);
    }

    @Override
    public void joinRoute(
            String sourceId, String presentationId, String origin, int tabId, int nativeRequestId) {
        mManager.onRouteRequestError(
                "Remote playback doesn't support joining routes", nativeRequestId);
    }

    @Override
    public void closeRoute(String routeId) {
        MediaRoute route = mRoutes.get(routeId);
        if (route == null) return;

        if (mSession == null) {
            mRoutes.remove(routeId);
            mManager.onRouteClosed(routeId);
            return;
        }

        ChromeCastSessionManager.get().stopApplication();
    }

    @Override
    public void detachRoute(String routeId) {
        mRoutes.remove(routeId);
    }

    @Override
    public void sendStringMessage(String routeId, String message, int nativeCallbackId) {
        Log.e(TAG, "Remote playback does not support sending messages");
        mManager.onMessageSentResult(false, nativeCallbackId);
    }

    @VisibleForTesting
    RemotingMediaRouteProvider(MediaRouter androidMediaRouter, MediaRouteManager manager) {
        super(androidMediaRouter, manager);
    }

    @Override
    public void onSessionEnded() {
        if (mSession == null) return;

        for (String routeId : mRoutes.keySet()) mManager.onRouteClosed(routeId);
        mRoutes.clear();

        mSession = null;

        if (mAndroidMediaRouter != null) {
            mAndroidMediaRouter.selectRoute(mAndroidMediaRouter.getDefaultRoute());
        }
    }

    @Override
    public void onSessionStarting(ChromeCastSessionManager.CastSessionLaunchRequest launchRequest) {
        CreateRouteRequest request = (CreateRouteRequest) launchRequest;
        MediaSink sink = request.getSink();
        MediaSource source = request.getSource();

        // Calling mManager.onRouteCreated() too early causes some issues. If we call it here
        // directly, getMediaController() might be called before onSessionStarted(), which causes
        // the FlingingRenderer's creation to fail. Instead, save the route and request ID, and only
        // signal the route as having been created when onSessionStarted() is called.
        mPendingMediaRoute =
                new MediaRoute(sink.getId(), source.getSourceId(), request.getPresentationId());
        mPendingNativeRequestId = request.getNativeRequestId();
    }

    private void clearPendingRoute() {
        mPendingMediaRoute = null;
        mPendingNativeRequestId = 0;
    }

    @Override
    public void onSessionStarted(CastSession session) {
        super.onSessionStarted(session);

        // Continued from onSessionStarting()
        mRoutes.put(mPendingMediaRoute.id, mPendingMediaRoute);
        mManager.onRouteCreated(mPendingMediaRoute.id, mPendingMediaRoute.sinkId,
                mPendingNativeRequestId, this, true);

        clearPendingRoute();
    }

    @Override
    public void onSessionStartFailed() {
        super.onSessionStartFailed();

        mManager.onRouteRequestError(
                "Failure to start RemotingCastSession", mPendingNativeRequestId);

        clearPendingRoute();
    };

    @Override
    @Nullable
    public MediaController getMediaController(String routeId) {
        // We cannot return a MediaController if we don't have a session.
        if (mSession == null) return null;

        // Don't return controllers for stale routes.
        if (mRoutes.get(routeId) == null) return null;

        // RemotePlayback does not support joining routes, which means we only
        // have a single route active at a time. If we have a a valid CastSession
        // and the route ID is current, this means that the given |mSession|
        // corresponds to the route ID, and it is ok to return the MediaController.
        return mSession.getMediaController();
    }
}
