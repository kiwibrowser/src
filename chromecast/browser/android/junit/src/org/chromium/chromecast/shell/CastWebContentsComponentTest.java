// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.support.v4.content.LocalBroadcastManager;

import org.junit.Assert;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowActivity;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chromecast.shell.CastWebContentsComponent.StartParams;
import org.chromium.content_public.browser.WebContents;

/**
 * Tests for CastWebContentsComponent.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CastWebContentsComponentTest {
    private static final String INSTANCE_ID = "1";

    private static final String APP_ID = "app";

    private static final int VISIBILITY_PRIORITY = 2;

    private @Mock WebContents mWebContents;
    private Activity mActivity;
    private ShadowActivity mShadowActivity;
    private StartParams mStartParams;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mActivity = Mockito.spy(Robolectric.buildActivity(Activity.class).setup().get());
        mShadowActivity = Shadows.shadowOf(mActivity);
        mStartParams = new StartParams(mActivity, mWebContents, APP_ID, VISIBILITY_PRIORITY);
    }

    @Test
    public void testStartStartsWebContentsActivity() {
        Assume.assumeFalse(BuildConfig.DISPLAY_WEB_CONTENTS_IN_SERVICE);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        component.start(mStartParams);
        Intent intent = mShadowActivity.getNextStartedActivity();
        Assert.assertEquals(
                intent.getComponent().getClassName(), CastWebContentsActivity.class.getName());

        component.stop(mActivity);
    }

    @Test
    public void testStopSendsStopSignalToActivity() {
        Assume.assumeFalse(BuildConfig.DISPLAY_WEB_CONTENTS_IN_SERVICE);

        BroadcastReceiver receiver = Mockito.mock(BroadcastReceiver.class);
        IntentFilter intentFilter = new IntentFilter(CastIntents.ACTION_STOP_WEB_CONTENT);
        LocalBroadcastManager.getInstance(ContextUtils.getApplicationContext())
                .registerReceiver(receiver, intentFilter);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        component.start(mStartParams);
        component.stop(ContextUtils.getApplicationContext());

        LocalBroadcastManager.getInstance(ContextUtils.getApplicationContext())
                .unregisterReceiver(receiver);

        verify(receiver).onReceive(any(Context.class), any(Intent.class));
    }

    @Test
    public void testStartBindsWebContentsService() {
        Assume.assumeTrue(BuildConfig.DISPLAY_WEB_CONTENTS_IN_SERVICE);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        component.start(mStartParams);
        component.stop(mActivity);

        ArgumentCaptor<Intent> intent = ArgumentCaptor.forClass(Intent.class);
        verify(mActivity).bindService(
                intent.capture(), any(ServiceConnection.class), eq(Context.BIND_AUTO_CREATE));
        Assert.assertEquals(intent.getValue().getComponent().getClassName(),
                CastWebContentsService.class.getName());
    }

    @Test
    public void testStopUnbindsWebContentsService() {
        Assume.assumeTrue(BuildConfig.DISPLAY_WEB_CONTENTS_IN_SERVICE);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        component.start(mStartParams);
        component.stop(mActivity);

        verify(mActivity).unbindService(any(ServiceConnection.class));
    }

    @Test
    public void testEnableTouchInputSendsEnableTouchToActivity() {
        Assume.assumeTrue(BuildConfig.DISPLAY_WEB_CONTENTS_IN_SERVICE);

        BroadcastReceiver receiver = Mockito.mock(BroadcastReceiver.class);
        IntentFilter intentFilter =
                new IntentFilter(CastWebContentsIntentUtils.ACTION_ENABLE_TOUCH_INPUT);
        LocalBroadcastManager.getInstance(ContextUtils.getApplicationContext())
                .registerReceiver(receiver, intentFilter);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        component.enableTouchInput(true);

        LocalBroadcastManager.getInstance(ContextUtils.getApplicationContext())
                .unregisterReceiver(receiver);

        verify(receiver).onReceive(any(Context.class), any(Intent.class));
    }

    @Test
    public void testEnableTouchInputBeforeStartedSendsEnableTouchToActivity() {
        Assume.assumeFalse(BuildConfig.DISPLAY_WEB_CONTENTS_IN_SERVICE);
        Assume.assumeFalse(BuildConfig.ENABLE_CAST_FRAGMENT);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        component.enableTouchInput(true);

        component.start(mStartParams);

        Intent intent = mShadowActivity.getNextStartedActivity();

        Assert.assertTrue(CastWebContentsIntentUtils.isTouchable(intent));
    }

    @Test
    public void testDisableTouchInputBeforeStartedSendsEnableTouchToActivity() {
        Assume.assumeFalse(BuildConfig.DISPLAY_WEB_CONTENTS_IN_SERVICE);
        Assume.assumeFalse(BuildConfig.ENABLE_CAST_FRAGMENT);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        component.enableTouchInput(false);

        component.start(mStartParams);

        Intent intent = mShadowActivity.getNextStartedActivity();

        Assert.assertFalse(CastWebContentsIntentUtils.isTouchable(intent));
    }

    @Test
    public void testOnComponentClosedCallsCallback() {
        CastWebContentsComponent.OnComponentClosedHandler callback =
                Mockito.mock(CastWebContentsComponent.OnComponentClosedHandler.class);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, callback, null, null, false, false);
        component.start(mStartParams);
        CastWebContentsComponent.onComponentClosed(INSTANCE_ID);
        verify(callback).onComponentClosed();

        component.stop(mActivity);
    }

    @Test
    public void testOnKeyDownCallsCallback() {
        CastWebContentsComponent.OnKeyDownHandler callback =
                Mockito.mock(CastWebContentsComponent.OnKeyDownHandler.class);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, callback, null, false, false);
        component.start(mStartParams);
        CastWebContentsComponent.onKeyDown(INSTANCE_ID, 42);
        component.stop(mActivity);

        verify(callback).onKeyDown(42);
    }

    @Test
    public void testStopDoesNotUnbindServiceIfStartWasNotCalled() {
        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);

        component.stop(mActivity);

        verify(mActivity, never()).unbindService(any(ServiceConnection.class));
    }

    @Test
    public void testOnVisibilityChangeCallback() {
        CastWebContentsComponent.SurfaceEventHandler callback =
                Mockito.mock(CastWebContentsComponent.SurfaceEventHandler.class);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, callback, false, false);
        component.start(mStartParams);
        CastWebContentsComponent.onVisibilityChange(INSTANCE_ID, 2);
        component.stop(mActivity);

        verify(callback).onVisibilityChange(2);
    }

    @Test
    public void testOnGestureCallback() {
        CastWebContentsComponent.SurfaceEventHandler callback =
                Mockito.mock(CastWebContentsComponent.SurfaceEventHandler.class);

        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, callback, false, false);
        component.start(mStartParams);
        CastWebContentsComponent.onGesture(INSTANCE_ID, 1);
        component.stop(mActivity);

        verify(callback).consumeGesture(1);
    }

    @Test
    public void testStartWebContentsComponentMultipleTimes() {
        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        CastWebContentsComponent.Delegate delegate = mock(CastWebContentsComponent.Delegate.class);
        component.setDelegate(delegate);
        component.start(mStartParams);
        Assert.assertTrue(component.isStarted());
        verify(delegate, times(1)).start(eq(mStartParams));
        StartParams params2 = new StartParams(mActivity, mWebContents, "test", 1);
        component.start(params2);
        Assert.assertTrue(component.isStarted());
        verify(delegate, times(2)).start(any(StartParams.class));
        verify(delegate, times(1)).start(eq(params2));
        component.stop(mActivity);
        Assert.assertFalse(component.isStarted());
        verify(delegate, times(1)).stop(any(Context.class));
    }

    @Test
    public void testStartActivityDelegateTwiceNoops() {
        // Sending focus events to a started Activity is unnecessary because the Activity is always
        // in focus, and issues with onNewIntent() and duplicate detection can cause unintended
        // side effects.
        CastWebContentsComponent component =
                new CastWebContentsComponent(INSTANCE_ID, null, null, null, false, false);
        component.setDelegate(component.new ActivityDelegate(false));
        component.start(mStartParams);
        Assert.assertEquals(mShadowActivity.getNextStartedActivity().getComponent().getClassName(),
                CastWebContentsActivity.class.getName());
        component.start(mStartParams);
        Assert.assertNull(mShadowActivity.getNextStartedActivity());
    }
}
