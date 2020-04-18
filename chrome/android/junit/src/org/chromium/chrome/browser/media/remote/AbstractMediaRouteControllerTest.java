// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.net.Uri;
import android.support.v7.media.MediaItemStatus;
import android.support.v7.media.MediaRouteSelector;
import android.support.v7.media.MediaRouter;
import android.support.v7.media.MediaRouter.Callback;
import android.support.v7.media.MediaRouter.ProviderInfo;
import android.support.v7.media.MediaRouter.RouteInfo;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.multidex.ShadowMultiDex;
import org.robolectric.util.ReflectionHelpers;

import org.chromium.base.CommandLine;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.media.remote.MediaRouteController.MediaStateListener;
import org.chromium.chrome.browser.media.remote.MediaRouteController.UiListener;

/** Tests for {@link AbstractMediaRouteController}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE,
        shadows = {AbstractMediaRouteControllerTest.ShadowMediaRouter.class, ShadowMultiDex.class})
public class AbstractMediaRouteControllerTest {
    /** Reset the environment before each test. */
    @Before
    public void beforeTest() {
        // TODO(dgn): Remove when command line flags are not used anymore to detect debug
        // see http://crbug.com/469649
        CommandLine.init(new String[] {});


        ShadowMediaRouter.sMediaRouter = null;
        ShadowMediaRouter.sCallback = null;
        DummyMediaRouteController.sMediaRouteSelector = mock(MediaRouteSelector.class);
    }

    /**
     * Test method for {@link AbstractMediaRouteController#isPlaying()}.
     *
     * Checks that it returns the correct value for all possible playback states.
     */
    @Test
    @Feature({"MediaRemote"})
    public void testIsPlaying() {
        // Using a spy here to override some methods.
        AbstractMediaRouteController mediaRouteCtrl = spy(new DummyMediaRouteController());

        // Default
        assertFalse(mediaRouteCtrl.isPlaying());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_BUFFERING);
        assertTrue(mediaRouteCtrl.isPlaying());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_CANCELED);
        assertFalse(mediaRouteCtrl.isPlaying());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_ERROR);
        assertFalse(mediaRouteCtrl.isPlaying());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_FINISHED);
        assertFalse(mediaRouteCtrl.isPlaying());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_INVALIDATED);
        assertFalse(mediaRouteCtrl.isPlaying());

        doReturn(0L).when(mediaRouteCtrl).getPosition();
        doReturn(5000L).when(mediaRouteCtrl).getDuration();
        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_PAUSED);
        assertFalse(mediaRouteCtrl.isPlaying());

        doReturn(5000L).when(mediaRouteCtrl).getPosition();
        doReturn(5000L).when(mediaRouteCtrl).getDuration();
        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_PAUSED);
        assertFalse(mediaRouteCtrl.isPlaying());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_PENDING);
        assertFalse(mediaRouteCtrl.isPlaying());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_PLAYING);
        assertTrue(mediaRouteCtrl.isPlaying());
    }

    /**
     * Test method for {@link AbstractMediaRouteController#isBeingCast()}.
     *
     * Checks that it returns the correct value for all possible playback states.
     */
    @Test
    @Feature({"MediaRemote"})
    public void testIsBeingCast() {
        // Using a spy here to override some methods.
        AbstractMediaRouteController mediaRouteCtrl = spy(new DummyMediaRouteController());

        // Default
        assertFalse(mediaRouteCtrl.isBeingCast());

        mediaRouteCtrl.setPreparedForTesting();

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_BUFFERING);
        assertTrue(mediaRouteCtrl.isBeingCast());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_CANCELED);
        assertFalse(mediaRouteCtrl.isBeingCast());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_ERROR);
        assertFalse(mediaRouteCtrl.isBeingCast());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_FINISHED);
        assertFalse(mediaRouteCtrl.isBeingCast());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_INVALIDATED);
        assertFalse(mediaRouteCtrl.isBeingCast());

        doReturn(0L).when(mediaRouteCtrl).getPosition();
        doReturn(5000L).when(mediaRouteCtrl).getDuration();
        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_PAUSED);
        assertTrue(mediaRouteCtrl.isBeingCast());

        doReturn(5000L).when(mediaRouteCtrl).getPosition();
        doReturn(5000L).when(mediaRouteCtrl).getDuration();
        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_PAUSED);
        assertFalse(mediaRouteCtrl.isBeingCast());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_PENDING);
        assertTrue(mediaRouteCtrl.isBeingCast());

        mediaRouteCtrl.setPlayerStateForMediaItemState(MediaItemStatus.PLAYBACK_STATE_PLAYING);
        assertTrue(mediaRouteCtrl.isBeingCast());

        mediaRouteCtrl.setUnprepared();
        assertFalse(mediaRouteCtrl.isBeingCast());
    }

    /** Test method for {@link AbstractMediaRouteController#isRemotePlaybackAvailable()}.*/
    @Test
    @Feature({"MediaRemote"})
    public void testIsRemotePlaybackAvailable() {
        MediaRouter mediaRouter = mock(MediaRouter.class);
        AbstractMediaRouteController mediaRouteCtrl = new DummyMediaRouteController();
        when(mediaRouter.getSelectedRoute())
                .thenReturn(createRouteInfo(MediaRouter.RouteInfo.PLAYBACK_TYPE_REMOTE));
        when(mediaRouter.isRouteAvailable(any(MediaRouteSelector.class), anyInt()))
                .thenReturn(true);

        // Default
        assertFalse(mediaRouteCtrl.isRemotePlaybackAvailable());

        ShadowMediaRouter.sMediaRouter = mediaRouter;
        mediaRouteCtrl = spy(new DummyMediaRouteController());
        assertTrue(mediaRouteCtrl.isRemotePlaybackAvailable());

        when(mediaRouter.getSelectedRoute())
                .thenReturn(createRouteInfo(MediaRouter.RouteInfo.PLAYBACK_TYPE_LOCAL));
        assertTrue(mediaRouteCtrl.isRemotePlaybackAvailable());

        when(mediaRouter.isRouteAvailable(any(MediaRouteSelector.class), anyInt()))
                .thenReturn(false);
        assertFalse(mediaRouteCtrl.isRemotePlaybackAvailable());

        when(mediaRouter.getSelectedRoute())
                .thenReturn(createRouteInfo(MediaRouter.RouteInfo.PLAYBACK_TYPE_REMOTE));
        assertTrue(mediaRouteCtrl.isRemotePlaybackAvailable());
    }

    /**
     * Test method for
     * {@link AbstractMediaRouteController#addMediaStateListener(MediaStateListener)} and
     * {@link AbstractMediaRouteController#removeMediaStateListener(MediaStateListener)}
     *
     * Makes sure that listeners gets notified when they are added and don't get notified
     * when removed.
     */
    @Test
    @Feature({"MediaRemote"})
    public void testAddRemoveMediaStateListener() {
        MediaStateListener listener = mock(MediaStateListener.class);
        MediaStateListener otherListener = mock(MediaStateListener.class);
        MediaRouter mediaRouter = ShadowMediaRouter.createCapturingMock();
        when(mediaRouter.isRouteAvailable(any(MediaRouteSelector.class), anyInt()))
                .thenReturn(true);
        ShadowMediaRouter.sMediaRouter = mediaRouter;

        // Creating the MediaRouteController
        AbstractMediaRouteController mediaRouteController = new DummyMediaRouteController();
        assertNotNull(mediaRouteController.getMediaRouter());

        // 1. #addMediaStateListener()
        mediaRouteController.addMediaStateListener(listener);

        // The route selector should get notified of new states.
        verify(mediaRouter)
                .addCallback(any(MediaRouteSelector.class), any(Callback.class), anyInt());
        verify(listener).onRouteAvailabilityChanged(true);

        // Check the behavior difference between the first and subsequent additions.
        mediaRouteController.addMediaStateListener(otherListener);
        verify(otherListener).onRouteAvailabilityChanged(true);
        // The call count should not change.
        verify(mediaRouter)
                .addCallback(any(MediaRouteSelector.class), any(Callback.class), anyInt());
        verify(listener).onRouteAvailabilityChanged(true);

        // 2. #removeMediaStateListener()
        mediaRouteController.removeMediaStateListener(otherListener);

        // The removed listener should not be notified of changes anymore.
        when(mediaRouter.isRouteAvailable(any(MediaRouteSelector.class), anyInt()))
                .thenReturn(false);
        ShadowMediaRouter.sCallback.onRouteRemoved(mediaRouter, null);
        verifyNoMoreInteractions(otherListener);
        verify(listener).onRouteAvailabilityChanged(false);
        verify(mediaRouter, times(0)).removeCallback(any(Callback.class));

        mediaRouteController.removeMediaStateListener(listener);

        when(mediaRouter.isRouteAvailable(any(MediaRouteSelector.class), anyInt()))
                .thenReturn(true);
        ShadowMediaRouter.sCallback.onRouteAdded(mediaRouter, null);
        verifyNoMoreInteractions(otherListener);
        verifyNoMoreInteractions(listener);
        verify(mediaRouter).removeCallback(any(Callback.class));
    }

    /**
     * Test method for
     * {@link AbstractMediaRouteController#addMediaStateListener(MediaStateListener)}
     *
     * Tests that listeners are not used (state not initialized) when the media router
     * is not initialized.
     */
    @Test
    @Feature({"MediaRemote"})
    public void testAddMediaStateListenerInitFailed() {
        MediaStateListener listener = mock(MediaStateListener.class);

        AbstractMediaRouteController mediaRouteController = new DummyMediaRouteController();
        mediaRouteController.addMediaStateListener(listener);

        verify(listener, never()).onRouteAvailabilityChanged(anyBoolean());
    }

    /** Test method for {@link AbstractMediaRouteController#prepareMediaRoute()}.*/
    @Test
    @Feature({"MediaRemote"})
    public void testPrepareMediaRoute() {
        // Check when no media router, check that not done twice

        ShadowMediaRouter.sMediaRouter = mock(MediaRouter.class);
        AbstractMediaRouteController mediaRouteCtrl = new DummyMediaRouteController();

        verify(ShadowMediaRouter.sMediaRouter, times(0))
                .addCallback(any(MediaRouteSelector.class), any(Callback.class), anyInt());

        mediaRouteCtrl.prepareMediaRoute();
        verify(ShadowMediaRouter.sMediaRouter, times(1))
                .addCallback(eq(DummyMediaRouteController.sMediaRouteSelector), any(Callback.class),
                        eq(MediaRouter.CALLBACK_FLAG_REQUEST_DISCOVERY));

        mediaRouteCtrl.prepareMediaRoute();
        verify(ShadowMediaRouter.sMediaRouter, times(1))
                .addCallback(any(MediaRouteSelector.class), any(Callback.class), anyInt());
    }

    /**
     * Test method for {@link AbstractMediaRouteController#addUiListener(UiListener)} and
     * {@link AbstractMediaRouteController#removeMediaStateListener(MediaStateListener)}.
     */
    @Test
    @Feature({"MediaRemote"})
    public void testAddRemoveUiListener() {
        DummyMediaRouteController mediaRouteCtrl = new DummyMediaRouteController();
        UiListener listener = mock(UiListener.class);

        mediaRouteCtrl.addUiListener(listener);
        mediaRouteCtrl.verifyListenerActivation(1, listener);

        // Should not be added twice.
        mediaRouteCtrl.addUiListener(listener);
        mediaRouteCtrl.verifyListenerActivation(2, listener);

        mediaRouteCtrl.removeUiListener(listener);
        mediaRouteCtrl.verifyListenerActivation(2, listener);
    }

    private static RouteInfo createRouteInfo(int playbackType) {
        Class<?>[] paramClasses = new Class[] {ProviderInfo.class, String.class, String.class};
        Object[] paramValues = new Object[] {null, "", ""};
        RouteInfo routeInfo = ReflectionHelpers.callConstructor(RouteInfo.class,
                ReflectionHelpers.ClassParameter.fromComponentLists(paramClasses, paramValues));
        ReflectionHelpers.setField(routeInfo, "mPlaybackType", playbackType);
        return routeInfo;
    }

    /** Shadow needed because getInstance() can't be mocked */
    @Implements(MediaRouter.class)
    public static class ShadowMediaRouter {
        public static MediaRouter sMediaRouter = null;
        public static Callback sCallback;

        @Implementation
        public static MediaRouter getInstance(Context context) {
            return sMediaRouter;
        }

        /**
         * Creates a {@link MediaRouter} mock that will capture the callback argument
         * when {@link MediaRouter#addCallback(MediaRouteSelector, Callback, int)} is called
         * and sets it to {@link #sCallback}.
         */
        public static MediaRouter createCapturingMock() {
            MediaRouter mediaRouter = mock(MediaRouter.class);

            final ArgumentCaptor<Callback> callbackArg = ArgumentCaptor.forClass(Callback.class);
            final Answer<?> addCallbackAnswer = new Answer<Object>() {
                @Override
                public Object answer(InvocationOnMock invocation) {
                    sCallback = callbackArg.getValue();
                    return null;
                }
            };

            Mockito.doAnswer(addCallbackAnswer)
                    .when(mediaRouter)
                    .addCallback(any(MediaRouteSelector.class), callbackArg.capture(), anyInt());
            return mediaRouter;
        }
    }

    /**
     * A dummy class used here to be able to instantiate the abstract class while calling its
     * constructor. {@link AbstractMediaRouteController#buildMediaRouteSelector()} needs to be
     * implemented before the object's construction. Mockito's mocks don't call constructors.
     */
    private static class DummyMediaRouteController extends AbstractMediaRouteController {
        /**
         * MediaRouteSelector required during MediaRouteController's constructor call via
         * {@link #buildMediaRouteSelector()}
         */
        public static MediaRouteSelector sMediaRouteSelector;

        @Override
        public boolean initialize() {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean canPlayMedia(String sourceUrl, String frameUrl) {
            throw new UnsupportedOperationException();
        }

        @Override
        public MediaRouteSelector buildMediaRouteSelector() {
            return sMediaRouteSelector;
        }

        @Override
        public boolean reconnectAnyExistingRoute() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void setDataSource(Uri uri, String cookies) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void prepareAsync(String frameUrl, long startPositionMillis) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void setRemoteVolume(int delta) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void resume() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void pause() {
            throw new UnsupportedOperationException();
        }

        @Override
        public long getPosition() {
            throw new UnsupportedOperationException();
        }

        @Override
        public long getDuration() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void seekTo(long msec) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void release() {
            throw new UnsupportedOperationException();
        }

        @Override
        protected void onRouteAddedEvent(MediaRouter router, RouteInfo route) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void onRouteSelected(MediaStateListener player, MediaRouter router,
                RouteInfo route) {
            throw new UnsupportedOperationException();
        }

        @Override
        protected void onRouteUnselectedEvent(MediaRouter router, RouteInfo route) {
            throw new UnsupportedOperationException();
        }

        public void verifyListenerActivation(int times, UiListener listener) {
            String testTitle = "foo";
            updateTitle(testTitle); // protected methods, needs to be called within the class.
            verify(listener, times(times)).onTitleChanged(testTitle);
        }
    }
}
