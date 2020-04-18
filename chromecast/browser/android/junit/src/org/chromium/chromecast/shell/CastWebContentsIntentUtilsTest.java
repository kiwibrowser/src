// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.net.Uri;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.content_public.browser.WebContents;

/**
 * Tests for CastWebContentsComponent.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CastWebContentsIntentUtilsTest {
    private static final String INSTANCE_ID = "1";
    private static final String EXPECTED_URI = "cast://webcontents/1";
    private static final String APP_ID = "app";
    private static final int VISIBILITY_PRIORITY = 2;

    private @Mock WebContents mWebContents;
    private @Mock BroadcastReceiver mReceiver;
    private Activity mActivity;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mActivity = Mockito.spy(Robolectric.buildActivity(Activity.class).setup().get());
    }

    @Test
    public void testOnActivityStopped() {
        Intent in = CastWebContentsIntentUtils.onActivityStopped(INSTANCE_ID);
        String uri = in.getDataString();
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        Assert.assertTrue(CastWebContentsIntentUtils.isIntentOfActivityStopped(in));
    }

    @Test
    public void testOnGesture() {
        Intent in = CastWebContentsIntentUtils.onGesture(INSTANCE_ID, 1);
        String uri = in.getDataString();
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        int type = CastWebContentsIntentUtils.getGestureType(in);
        Assert.assertEquals(1, type);
        Assert.assertTrue(CastWebContentsIntentUtils.isIntentOfGesturing(in));

        in = CastWebContentsIntentUtils.onGestureWithUriString(EXPECTED_URI, 2);
        uri = in.getDataString();
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        type = CastWebContentsIntentUtils.getGestureType(in);
        Assert.assertEquals(2, type);
        Assert.assertTrue(CastWebContentsIntentUtils.isIntentOfGesturing(in));
    }

    @Test
    public void testOnKeyDown() {
        Intent in = CastWebContentsIntentUtils.onKeyDown(INSTANCE_ID, 32);
        String uri = in.getDataString();
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        int type = CastWebContentsIntentUtils.getKeyCode(in);
        Assert.assertEquals(32, type);
        Assert.assertTrue(CastWebContentsIntentUtils.isIntentOfKeyEvent(in));
    }

    @Test
    public void testOnVisibilityChange() {
        Intent in = CastWebContentsIntentUtils.onVisibilityChange(INSTANCE_ID, 3);
        String uri = in.getDataString();
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        int type = CastWebContentsIntentUtils.getVisibilityType(in);
        Assert.assertEquals(3, type);
        Assert.assertTrue(CastWebContentsIntentUtils.isIntentOfVisibilityChange(in));

        in = CastWebContentsIntentUtils.onVisibilityChangeWithUriString(EXPECTED_URI, 2);
        uri = in.getDataString();
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        type = CastWebContentsIntentUtils.getVisibilityType(in);
        Assert.assertEquals(2, type);
        Assert.assertTrue(CastWebContentsIntentUtils.isIntentOfVisibilityChange(in));
    }

    @Test
    public void testRequestVisibilityPriority() {
        Intent in = CastWebContentsIntentUtils.requestVisibilityPriority(INSTANCE_ID, 2);
        Assert.assertNull(in.getData());
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        int type = CastWebContentsIntentUtils.getVisibilityPriority(in);
        Assert.assertEquals(2, type);
        Assert.assertTrue(CastWebContentsIntentUtils.isIntentToRequestVisibilityPriority(in));
    }

    @Test
    public void testRequestMoveOut() {
        Intent in = CastWebContentsIntentUtils.requestMoveOut(INSTANCE_ID);
        Assert.assertNull(in.getData());
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        Assert.assertTrue(CastWebContentsIntentUtils.isIntentToRequestMoveOut(in));
    }

    @Test
    public void testGestureConsumed() {
        Intent in = CastWebContentsIntentUtils.gestureConsumed(INSTANCE_ID, 1, true);
        Assert.assertNull(in.getData());
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertEquals(EXPECTED_URI, uri);
        int type = CastWebContentsIntentUtils.getGestureType(in);
        Assert.assertEquals(1, type);
        Assert.assertTrue(CastWebContentsIntentUtils.isGestureConsumed(in));
        Assert.assertEquals(CastWebContentsIntentUtils.ACTION_GESTURE_CONSUMED, in.getAction());

        in = CastWebContentsIntentUtils.gestureConsumed(INSTANCE_ID, 2, false);
        Assert.assertNull(in.getData());
        uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        type = CastWebContentsIntentUtils.getGestureType(in);
        Assert.assertEquals(2, type);
        Assert.assertFalse(CastWebContentsIntentUtils.isGestureConsumed(in));
    }

    @Test
    public void testRequestStartCastActivity() {
        Intent in = CastWebContentsIntentUtils.requestStartCastActivity(
                mActivity, mWebContents, true, INSTANCE_ID);
        Assert.assertNull(in.getData());
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        WebContents webContents = CastWebContentsIntentUtils.getWebContents(in);
        Assert.assertEquals(mWebContents, webContents);
        Assert.assertTrue(CastWebContentsIntentUtils.isTouchable(in));
        Assert.assertEquals(Intent.ACTION_VIEW, in.getAction());
    }

    @Test
    public void testRequestStartCastFragment() {
        Intent in = CastWebContentsIntentUtils.requestStartCastFragment(
                mWebContents, APP_ID, 3, true, INSTANCE_ID);
        Assert.assertNull(in.getData());
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        WebContents webContents = CastWebContentsIntentUtils.getWebContents(in);
        Assert.assertEquals(mWebContents, webContents);
        Assert.assertTrue(CastWebContentsIntentUtils.isTouchable(in));
        Assert.assertEquals(APP_ID, CastWebContentsIntentUtils.getAppId(in));
        Assert.assertEquals(3, CastWebContentsIntentUtils.getVisibilityPriority(in));
        Assert.assertEquals(CastIntents.ACTION_SHOW_WEB_CONTENT, in.getAction());
    }

    @Test
    public void testRequestStartCastService() {
        Intent in = CastWebContentsIntentUtils.requestStartCastService(
                mActivity, mWebContents, INSTANCE_ID);
        String uri = in.getDataString();
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        WebContents webContents = CastWebContentsIntentUtils.getWebContents(in);
        Assert.assertEquals(mWebContents, webContents);
        Assert.assertEquals(Intent.ACTION_VIEW, in.getAction());
    }

    @Test
    public void testRequestStopWebContents() {
        Intent in = CastWebContentsIntentUtils.requestStopWebContents(INSTANCE_ID);
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
    }

    @Test
    public void testEnableTouchInputTrue() {
        Intent in = CastWebContentsIntentUtils.enableTouchInput(INSTANCE_ID, true);
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        Assert.assertEquals(CastWebContentsIntentUtils.ACTION_ENABLE_TOUCH_INPUT, in.getAction());
        Assert.assertTrue(CastWebContentsIntentUtils.isTouchable(in));
    }

    @Test
    public void testEnableTouchInputFalse() {
        Intent in = CastWebContentsIntentUtils.enableTouchInput(INSTANCE_ID, false);
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
        Assert.assertEquals(CastWebContentsIntentUtils.ACTION_ENABLE_TOUCH_INPUT, in.getAction());
        Assert.assertFalse(CastWebContentsIntentUtils.isTouchable(in));
    }

    @Test
    public void testOnWebContentStopped() {
        Intent in = CastWebContentsIntentUtils.onWebContentStopped(Uri.parse(EXPECTED_URI));
        String uri = CastWebContentsIntentUtils.getUriString(in);
        Assert.assertNotNull(uri);
        Assert.assertEquals(EXPECTED_URI, uri);
    }
}
