// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.same;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.support.v7.media.MediaRouter;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.media.router.ChromeMediaRouter;
import org.chromium.chrome.browser.media.router.MediaRoute;
import org.chromium.chrome.browser.media.router.MediaRouteManager;
import org.chromium.chrome.browser.media.router.MediaSink;

import java.util.ArrayList;

/**
 * Robolectric tests for {@link CastMediaRouteProvider}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CastMediaRouteProviderTest {
    private static final String SUPPORTED_SOURCE = "cast:DEADBEEF";

    private static final String SUPPORTED_AUTOJOIN_SOURCE = "cast:DEADBEEF"
            + "?clientId=12345&autoJoinPolicy=" + CastMediaSource.AUTOJOIN_TAB_AND_ORIGIN_SCOPED;

    // TODO(crbug.com/672704): Android does not currently support 1-UA mode.
    private static final String UNSUPPORTED_SOURCE = "https://example.com";

    private MediaRouteManager mMockManager;
    private CastMediaRouteProvider mProvider;

    protected void setUpMediaRouter(MediaRouter router) {
        ChromeMediaRouter.setAndroidMediaRouterForTest(router);
        mMockManager = mock(MediaRouteManager.class);
        mProvider = CastMediaRouteProvider.create(mMockManager);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testStartObservingMediaSinksNoMediaRouter() {
        setUpMediaRouter(null);

        mProvider.startObservingMediaSinks(SUPPORTED_SOURCE);

        verify(mMockManager, timeout(100))
                .onSinksReceived(
                        eq(SUPPORTED_SOURCE), same(mProvider), eq(new ArrayList<MediaSink>()));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testStartObservingMediaSinksUnsupportedSource() {
        setUpMediaRouter(mock(MediaRouter.class));

        mProvider.startObservingMediaSinks(UNSUPPORTED_SOURCE);

        verify(mMockManager, timeout(100))
                .onSinksReceived(
                        eq(UNSUPPORTED_SOURCE), same(mProvider), eq(new ArrayList<MediaSink>()));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testOnSessionClosedNoClientRecord() {
        setUpMediaRouter(mock(MediaRouter.class));

        CastSession mockSession = mock(CastSession.class);
        mProvider.onSessionStarted(mockSession);

        MediaRoute route = new MediaRoute("sink", SUPPORTED_SOURCE, "");
        mProvider.addRoute(route, "", -1);
        mProvider.onSessionEnded();

        verify(mMockManager).onRouteClosed(route.id);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCloseRouteWithNoSession() {
        setUpMediaRouter(mock(MediaRouter.class));

        MediaRoute route = new MediaRoute("sink", SUPPORTED_SOURCE, "");
        mProvider.addRoute(route, "", -1);
        mProvider.closeRoute(route.id);

        verify(mMockManager).onRouteClosed(route.id);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testAutoJoinWithSameOrigin() {
        setUpMediaRouter(mock(MediaRouter.class));

        CastSession mockSession = mock(CastSession.class);
        when(mockSession.getSinkId()).thenReturn("sinkId");
        when(mockSession.getSourceId()).thenReturn(SUPPORTED_AUTOJOIN_SOURCE);

        MediaRoute route = new MediaRoute("sinkId", SUPPORTED_AUTOJOIN_SOURCE, "presentationId");
        mProvider.addRoute(route, "https://example.com", 1);
        mProvider.onSessionStarted(mockSession);
        mProvider.joinRoute(SUPPORTED_AUTOJOIN_SOURCE, "auto-join", "https://example.com", 1, -1);

        verify(mMockManager)
                .onRouteCreated("route:auto-join/sinkId/" + SUPPORTED_AUTOJOIN_SOURCE, "sinkId", -1,
                        mProvider, false);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testNoAutoJoinWithOneUniqueOrigin() {
        setUpMediaRouter(mock(MediaRouter.class));

        CastSession mockSession = mock(CastSession.class);
        when(mockSession.getSinkId()).thenReturn("sinkId");
        when(mockSession.getSourceId()).thenReturn(SUPPORTED_AUTOJOIN_SOURCE);

        MediaRoute route = new MediaRoute("sinkId", SUPPORTED_AUTOJOIN_SOURCE, "presentationId");
        mProvider.addRoute(route, "https://example.com", 1);
        mProvider.onSessionStarted(mockSession);
        mProvider.joinRoute(SUPPORTED_AUTOJOIN_SOURCE, "auto-join", "", 1, -1);

        verify(mMockManager).onRouteRequestError("No matching route", -1);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testNoAutoJoinWithTwoUniqueOrigins() {
        setUpMediaRouter(mock(MediaRouter.class));

        CastSession mockSession = mock(CastSession.class);
        when(mockSession.getSinkId()).thenReturn("sinkId");
        when(mockSession.getSourceId()).thenReturn(SUPPORTED_AUTOJOIN_SOURCE);

        MediaRoute route = new MediaRoute("sinkId", SUPPORTED_AUTOJOIN_SOURCE, "presentationId");
        mProvider.addRoute(route, "", 1);
        mProvider.onSessionStarted(mockSession);
        mProvider.joinRoute(SUPPORTED_AUTOJOIN_SOURCE, "auto-join", "", 1, -1);

        verify(mMockManager).onRouteRequestError("No matching route", -1);
    }
}
