// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.net.Uri;
import android.os.PatternMatcher;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chromecast.base.Consumer;
import org.chromium.chromecast.base.Scope;
import org.chromium.chromecast.base.ScopeFactory;
import org.chromium.chromecast.shell.CastWebContentsSurfaceHelper.ContentVideoViewEmbedderSetter;
import org.chromium.chromecast.shell.CastWebContentsSurfaceHelper.MediaSessionGetter;
import org.chromium.chromecast.shell.CastWebContentsSurfaceHelper.StartParams;
import org.chromium.content.browser.MediaSessionImpl;
import org.chromium.content_public.browser.ContentVideoViewEmbedder;
import org.chromium.content_public.browser.WebContents;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for CastWebContentsSurfaceHelper.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CastWebContentsSurfaceHelperTest {
    private @Mock Activity mActivity;
    private @Mock ScopeFactory<WebContents> mWebContentsView;
    private @Mock Consumer<Uri> mFinishCallback;
    private CastWebContentsSurfaceHelper mSurfaceHelper;
    private @Mock ContentVideoViewEmbedderSetter mContentVideoViewEmbedderSetter;
    private @Mock MediaSessionGetter mMediaSessionGetter;
    private @Mock MediaSessionImpl mMediaSessionImpl;

    private static class StartParamsBuilder {
        private String mId = "0";
        private WebContents mWebContents = mock(WebContents.class);
        private boolean mIsTouchInputEnabled = false;

        public StartParamsBuilder withId(String id) {
            mId = id;
            return this;
        }

        public StartParamsBuilder withWebContents(WebContents webContents) {
            mWebContents = webContents;
            return this;
        }

        public StartParamsBuilder enableTouchInput(boolean enableTouchInput) {
            mIsTouchInputEnabled = enableTouchInput;
            return this;
        }

        public StartParams build() {
            return new StartParams(CastWebContentsIntentUtils.getInstanceUri(mId), mWebContents,
                    mIsTouchInputEnabled);
        }
    }

    private static class BroadcastAsserter {
        private final Intent mExpectedIntent;
        private final List<Intent> mReceivedIntents = new ArrayList<>();
        private final LocalBroadcastReceiverScope mReceiver;

        public BroadcastAsserter(Intent looksLike) {
            mExpectedIntent = looksLike;
            IntentFilter filter = new IntentFilter();
            Uri instanceUri = looksLike.getData();
            filter.addDataScheme(instanceUri.getScheme());
            filter.addDataAuthority(instanceUri.getAuthority(), null);
            filter.addDataPath(instanceUri.getPath(), PatternMatcher.PATTERN_LITERAL);
            filter.addAction(looksLike.getAction());
            mReceiver = new LocalBroadcastReceiverScope(filter, mReceivedIntents::add);
        }

        public void verify() {
            assertEquals(1, mReceivedIntents.size());
            Intent receivedIntent = mReceivedIntents.get(0);
            assertEquals(mExpectedIntent.getAction(), receivedIntent.getAction());
            assertEquals(mExpectedIntent.getData(), receivedIntent.getData());
            mReceiver.close();
        }
    }

    private void sendBroadcastSync(Intent intent) {
        CastWebContentsIntentUtils.getLocalBroadcastManager().sendBroadcastSync(intent);
    }

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        when(mMediaSessionGetter.get(any())).thenReturn(mMediaSessionImpl);
        when(mWebContentsView.create(any())).thenReturn(mock(Scope.class));
        mSurfaceHelper =
                new CastWebContentsSurfaceHelper(mActivity, mWebContentsView, mFinishCallback);
        mSurfaceHelper.setContentVideoViewEmbedderSetterForTesting(mContentVideoViewEmbedderSetter);
        mSurfaceHelper.setMediaSessionGetterForTesting(mMediaSessionGetter);
    }

    @Test
    public void testActivatesWebContentsViewOnNewStartParams() {
        WebContents webContents = mock(WebContents.class);
        StartParams params = new StartParamsBuilder().withWebContents(webContents).build();
        mSurfaceHelper.onNewStartParams(params);
        verify(mWebContentsView).create(webContents);
    }

    @Test
    public void testRequestsAudioFocusOnNewStartParams() {
        WebContents webContents = mock(WebContents.class);
        StartParams params = new StartParamsBuilder().withWebContents(webContents).build();
        mSurfaceHelper.onNewStartParams(params);
        verify(mMediaSessionImpl).requestSystemAudioFocus();
    }

    @Test
    public void testDeactivatesOldWebContentsViewOnNewStartParams() {
        WebContents webContents1 = mock(WebContents.class);
        StartParams params1 =
                new StartParamsBuilder().withId("1").withWebContents(webContents1).build();
        WebContents webContents2 = mock(WebContents.class);
        StartParams params2 =
                new StartParamsBuilder().withId("2").withWebContents(webContents2).build();
        Scope scope1 = mock(Scope.class);
        Scope scope2 = mock(Scope.class);
        when(mWebContentsView.create(webContents1)).thenReturn(scope1);
        when(mWebContentsView.create(webContents2)).thenReturn(scope2);
        mSurfaceHelper.onNewStartParams(params1);
        verify(mWebContentsView).create(webContents1);
        mSurfaceHelper.onNewStartParams(params2);
        verify(scope1).close();
        verify(mWebContentsView).create(webContents2);
    }

    @Test
    public void testSetsContentVideoViewEmbedderOnNewStartParams() {
        WebContents webContents = mock(WebContents.class);
        StartParams params = new StartParamsBuilder().withWebContents(webContents).build();
        mSurfaceHelper.onNewStartParams(params);
        verify(mContentVideoViewEmbedderSetter)
                .set(eq(webContents), any(ContentVideoViewEmbedder.class));
    }

    @Test
    public void testIsTouchInputEnabled() {
        assertFalse(mSurfaceHelper.isTouchInputEnabled());
        StartParams params1 = new StartParamsBuilder().enableTouchInput(true).build();
        mSurfaceHelper.onNewStartParams(params1);
        assertTrue(mSurfaceHelper.isTouchInputEnabled());
        StartParams params2 = new StartParamsBuilder().enableTouchInput(false).build();
        mSurfaceHelper.onNewStartParams(params2);
        assertFalse(mSurfaceHelper.isTouchInputEnabled());
    }

    @Test
    public void testInstanceId() {
        assertNull(mSurfaceHelper.getInstanceId());
        StartParams params1 = new StartParamsBuilder().withId("/0").build();
        mSurfaceHelper.onNewStartParams(params1);
        assertEquals("/0", mSurfaceHelper.getInstanceId());
        StartParams params2 = new StartParamsBuilder().withId("/1").build();
        mSurfaceHelper.onNewStartParams(params2);
        assertEquals("/1", mSurfaceHelper.getInstanceId());
    }

    @Test
    public void testScreenOffResetsWebContentsView() {
        WebContents webContents = mock(WebContents.class);
        StartParams params = new StartParamsBuilder().withWebContents(webContents).build();
        Scope scope = mock(Scope.class);
        when(mWebContentsView.create(webContents)).thenReturn(scope);
        mSurfaceHelper.onNewStartParams(params);
        // Send SCREEN_OFF broadcast.
        sendBroadcastSync(new Intent(CastIntents.ACTION_SCREEN_OFF));
        verify(scope).close();
    }

    @Test
    public void testStopWebContentsIntentResetsWebContentsView() {
        WebContents webContents = mock(WebContents.class);
        StartParams params =
                new StartParamsBuilder().withId("3").withWebContents(webContents).build();
        Scope scope = mock(Scope.class);
        when(mWebContentsView.create(webContents)).thenReturn(scope);
        mSurfaceHelper.onNewStartParams(params);
        // Send notification to stop web content
        sendBroadcastSync(CastWebContentsIntentUtils.requestStopWebContents("3"));
        verify(scope).close();
    }

    @Test
    public void testStopWebContentsIntentWithWrongIdIsIgnored() {
        WebContents webContents = mock(WebContents.class);
        StartParams params =
                new StartParamsBuilder().withId("2").withWebContents(webContents).build();
        Scope scope = mock(Scope.class);
        when(mWebContentsView.create(webContents)).thenReturn(scope);
        mSurfaceHelper.onNewStartParams(params);
        // Send notification to stop web content with different ID.
        sendBroadcastSync(CastWebContentsIntentUtils.requestStopWebContents("4"));
        verify(scope, never()).close();
    }

    @Test
    public void testEnableTouchInputIntentMutatesIsTouchInputEnabled() {
        WebContents webContents = mock(WebContents.class);
        StartParams params = new StartParamsBuilder().withId("1").enableTouchInput(false).build();
        mSurfaceHelper.onNewStartParams(params);
        assertFalse(mSurfaceHelper.isTouchInputEnabled());
        // Send broadcast to enable touch input.
        sendBroadcastSync(CastWebContentsIntentUtils.enableTouchInput("1", true));
        assertTrue(mSurfaceHelper.isTouchInputEnabled());
    }

    @Test
    public void testEnableTouchInputIntentWithWrongIdIsIgnored() {
        WebContents webContents = mock(WebContents.class);
        StartParams params = new StartParamsBuilder().withId("1").enableTouchInput(false).build();
        mSurfaceHelper.onNewStartParams(params);
        assertFalse(mSurfaceHelper.isTouchInputEnabled());
        // Send broadcast to enable touch input with different ID.
        sendBroadcastSync(CastWebContentsIntentUtils.enableTouchInput("2", true));
        assertFalse(mSurfaceHelper.isTouchInputEnabled());
    }

    @Test
    public void testDisableTouchInputIntent() {
        WebContents webContents = mock(WebContents.class);
        StartParams params = new StartParamsBuilder().withId("1").enableTouchInput(true).build();
        mSurfaceHelper.onNewStartParams(params);
        assertTrue(mSurfaceHelper.isTouchInputEnabled());
        // Send broadcast to enable touch input.
        sendBroadcastSync(CastWebContentsIntentUtils.enableTouchInput("1", false));
        assertFalse(mSurfaceHelper.isTouchInputEnabled());
    }

    @Test
    public void testSetsVolumeControlStreamOfHostActivity() {
        StartParams params = new StartParamsBuilder().build();
        mSurfaceHelper.onNewStartParams(params);
        verify(mActivity).setVolumeControlStream(AudioManager.STREAM_MUSIC);
    }

    @Test
    public void testSendsComponentClosedBroadcastWhenWebContentsViewIsClosedAlt() {
        StartParams params1 = new StartParamsBuilder().withId("1").build();
        StartParams params2 = new StartParamsBuilder().withId("2").build();
        mSurfaceHelper.onNewStartParams(params1);
        // Listen for notification from surface helper that web contents closed.
        BroadcastAsserter intentWasSent =
                new BroadcastAsserter(CastWebContentsIntentUtils.onActivityStopped("1"));
        mSurfaceHelper.onNewStartParams(params2);
        intentWasSent.verify();
    }

    @Test
    public void testFinishLaterCallbackIsRunAfterStopWebContents() {
        StartParams params = new StartParamsBuilder().withId("0").build();
        mSurfaceHelper.onNewStartParams(params);
        sendBroadcastSync(CastWebContentsIntentUtils.requestStopWebContents("0"));
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        verify(mFinishCallback).accept(CastWebContentsIntentUtils.getInstanceUri("0"));
    }

    @Test
    public void testFinishLaterCallbackIsRunAfterScreenOff() {
        StartParams params = new StartParamsBuilder().withId("0").build();
        mSurfaceHelper.onNewStartParams(params);
        sendBroadcastSync(new Intent(CastIntents.ACTION_SCREEN_OFF));
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        verify(mFinishCallback).accept(CastWebContentsIntentUtils.getInstanceUri("0"));
    }

    @Test
    public void testFinishLaterCallbackIsNotRunIfNewWebContentsIsReceived() {
        StartParams params1 = new StartParamsBuilder().withId("1").build();
        StartParams params2 = new StartParamsBuilder().withId("2").build();
        mSurfaceHelper.onNewStartParams(params1);
        sendBroadcastSync(CastWebContentsIntentUtils.requestStopWebContents("1"));
        mSurfaceHelper.onNewStartParams(params2);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        verify(mFinishCallback, never()).accept(any());
    }

    @Test
    public void testOnDestroyClosesWebContentsView() {
        WebContents webContents = mock(WebContents.class);
        Scope scope = mock(Scope.class);
        StartParams params = new StartParamsBuilder().withWebContents(webContents).build();
        when(mWebContentsView.create(webContents)).thenReturn(scope);
        mSurfaceHelper.onNewStartParams(params);
        mSurfaceHelper.onDestroy();
        verify(scope).close();
    }

    @Test
    public void testOnDestroyNotifiesComponent() {
        StartParams params = new StartParamsBuilder().withId("2").build();
        mSurfaceHelper.onNewStartParams(params);
        BroadcastAsserter intentWasSent =
                new BroadcastAsserter(CastWebContentsIntentUtils.onActivityStopped("2"));
        mSurfaceHelper.onDestroy();
        intentWasSent.verify();
    }
}
