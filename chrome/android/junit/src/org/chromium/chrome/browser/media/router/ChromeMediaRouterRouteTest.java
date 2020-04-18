// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.verify;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;

/**
 * Route tests for ChromeMediaRouter.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ChromeMediaRouterRouteTest extends ChromeMediaRouterTestBase {
    @Test
    @Feature({"MediaRouter"})
    public void testCreateOneRoute() {
        assertEquals(mChromeMediaRouter.getRouteIdsToProvidersForTest().size(), 0);

        mChromeMediaRouter.createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);
        verify(mRouteProvider).createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);

        String routeId1 = new MediaRoute(SINK_ID1, SOURCE_ID1, PRESENTATION_ID1).id;
        mChromeMediaRouter.onRouteCreated(
                routeId1, SINK_ID1, REQUEST_ID1, mRouteProvider, true);

        assertEquals(1, mChromeMediaRouter.getRouteIdsToProvidersForTest().size());
        assertTrue(mChromeMediaRouter.getRouteIdsToProvidersForTest().containsKey(routeId1));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCreateTwoRoutes() {
        mChromeMediaRouter.createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);

        String routeId1 = new MediaRoute(SINK_ID1, SOURCE_ID1, PRESENTATION_ID1).id;
        mChromeMediaRouter.onRouteCreated(
                routeId1, SINK_ID1, REQUEST_ID1, mRouteProvider, true);

        mChromeMediaRouter.createRoute(
                SOURCE_ID2, SINK_ID2, PRESENTATION_ID2, ORIGIN2, TAB_ID2, false, REQUEST_ID2);

        verify(mRouteProvider).createRoute(
                SOURCE_ID2, SINK_ID2, PRESENTATION_ID2, ORIGIN2, TAB_ID2, false, REQUEST_ID2);
        String routeId2 = new MediaRoute(SINK_ID2, SOURCE_ID2, PRESENTATION_ID2).id;
        mChromeMediaRouter.onRouteCreated(
                routeId2, SINK_ID2, REQUEST_ID2, mRouteProvider, true);

        assertEquals(2, mChromeMediaRouter.getRouteIdsToProvidersForTest().size());
        assertTrue(mChromeMediaRouter.getRouteIdsToProvidersForTest().containsKey(routeId2));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCreateRouteFails() {
        mChromeMediaRouter.createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);

        verify(mRouteProvider).createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);
        mChromeMediaRouter.onRouteRequestError("ERROR", REQUEST_ID1);

        assertEquals(0, mChromeMediaRouter.getRouteIdsToProvidersForTest().size());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testJoinRoute() {
        mChromeMediaRouter.createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);

        String routeId1 = new MediaRoute(SINK_ID1, SOURCE_ID1, PRESENTATION_ID1).id;
        mChromeMediaRouter.onRouteCreated(
                routeId1, SINK_ID1, REQUEST_ID1, mRouteProvider, true);

        mChromeMediaRouter.joinRoute(
                SOURCE_ID2, PRESENTATION_ID1, ORIGIN1, TAB_ID2, REQUEST_ID2);
        verify(mRouteProvider).joinRoute(
                SOURCE_ID2, PRESENTATION_ID1, ORIGIN1, TAB_ID2, REQUEST_ID2);

        String routeId2 = new MediaRoute(SINK_ID1, SOURCE_ID2, PRESENTATION_ID2).id;
        mChromeMediaRouter.onRouteCreated(
                routeId2, SINK_ID1, REQUEST_ID2, mRouteProvider, true);

        assertEquals(2, mChromeMediaRouter.getRouteIdsToProvidersForTest().size());
        assertTrue(mChromeMediaRouter.getRouteIdsToProvidersForTest().containsKey(routeId2));
    }

    @Test
    @Feature({"MediaRouter"})
    public void testJoinRouteFails() {
        mChromeMediaRouter.createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);

        String routeId1 = new MediaRoute(SINK_ID1, SOURCE_ID1, PRESENTATION_ID1).id;
        mChromeMediaRouter.onRouteCreated(
                routeId1, SINK_ID1, REQUEST_ID1, mRouteProvider, true);

        mChromeMediaRouter.joinRoute(
                SOURCE_ID2, PRESENTATION_ID1, ORIGIN1, TAB_ID2, REQUEST_ID2);
        verify(mRouteProvider).joinRoute(
                SOURCE_ID2, PRESENTATION_ID1, ORIGIN1, TAB_ID2, REQUEST_ID2);

        mChromeMediaRouter.onRouteRequestError("error", REQUEST_ID2);

        assertEquals(1, mChromeMediaRouter.getRouteIdsToProvidersForTest().size());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testDetachRoute() {
        mChromeMediaRouter.createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);

        String routeId1 = new MediaRoute(SINK_ID1, SOURCE_ID1, PRESENTATION_ID1).id;
        mChromeMediaRouter.onRouteCreated(
                routeId1, SINK_ID1, REQUEST_ID1, mRouteProvider, true);

        mChromeMediaRouter.detachRoute(routeId1);
        verify(mRouteProvider).detachRoute(routeId1);

        assertEquals(0, mChromeMediaRouter.getRouteIdsToProvidersForTest().size());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testCloseRoute() {
        mChromeMediaRouter.createRoute(
                SOURCE_ID1, SINK_ID1, PRESENTATION_ID1, ORIGIN1, TAB_ID1, false, REQUEST_ID1);

        String routeId1 = new MediaRoute(SINK_ID1, SOURCE_ID1, PRESENTATION_ID1).id;
        mChromeMediaRouter.onRouteCreated(
                routeId1, SINK_ID1, REQUEST_ID1, mRouteProvider, true);

        mChromeMediaRouter.closeRoute(routeId1);
        verify(mRouteProvider).closeRoute(routeId1);
        assertEquals(1, mChromeMediaRouter.getRouteIdsToProvidersForTest().size());

        mChromeMediaRouter.onRouteClosed(routeId1);
        assertEquals(0, mChromeMediaRouter.getRouteIdsToProvidersForTest().size());
    }
}
