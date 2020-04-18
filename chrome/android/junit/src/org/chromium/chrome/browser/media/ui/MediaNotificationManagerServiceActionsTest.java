// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.ui;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;

import android.content.Intent;
import android.media.AudioManager;
import android.view.KeyEvent;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.blink.mojom.MediaSessionAction;
import org.chromium.chrome.browser.media.ui.MediaNotificationManager.ListenerService;

/**
 * JUnit tests for checking {@link MediaNotificationManager.ListenerService} handles intent actionss
 * correctly.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE,
        shadows = {MediaNotificationTestShadowResources.class,
                MediaNotificationTestShadowNotificationManager.class})
public class MediaNotificationManagerServiceActionsTest extends MediaNotificationManagerTestBase {
    @Test
    public void testProcessIntentWithNoAction() {
        setUpServiceAndClearInvocations();
        doNothing().when(getManager()).onServiceStarted(any(ListenerService.class));
        assertTrue(mService.processIntent(new Intent()));
        verify(getManager()).onServiceStarted(mService);
    }

    @Test
    public void testProcessIntentWithAction() {
        setUpService();
        doNothing().when(getManager()).onServiceStarted(any(ListenerService.class));
        Intent intentWithAction = new Intent().setAction("foo");
        assertTrue(mService.processIntent(intentWithAction));
        verify(mService).processAction(intentWithAction, getManager());
    }

    @Test
    public void testProcessMediaButton_Play() {
        setUpService();

        mService.processAction(
                createMediaButtonActionIntent(KeyEvent.KEYCODE_MEDIA_PLAY), getManager());
        verify(getManager()).onPlay(MediaNotificationListener.ACTION_SOURCE_MEDIA_SESSION);
    }

    @Test
    public void testProcessMediaButton_Pause() {
        setUpService();

        mService.processAction(
                createMediaButtonActionIntent(KeyEvent.KEYCODE_MEDIA_PAUSE), getManager());
        verify(getManager()).onPause(MediaNotificationListener.ACTION_SOURCE_MEDIA_SESSION);
    }

    @Test
    public void testProcessMediaButton_HeadsetHook() {
        setUpService();

        mMediaNotificationInfoBuilder.setPaused(false);
        getManager().mMediaNotificationInfo = mMediaNotificationInfoBuilder.build();

        mService.processAction(
                createMediaButtonActionIntent(KeyEvent.KEYCODE_HEADSETHOOK), getManager());
        verify(getManager()).onPause(MediaNotificationListener.ACTION_SOURCE_MEDIA_SESSION);
    }

    @Test
    public void testProcessMediaButton_PlayPause() {
        setUpService();

        mService.processAction(
                createMediaButtonActionIntent(KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE), getManager());
        verify(getManager()).onPause(MediaNotificationListener.ACTION_SOURCE_MEDIA_SESSION);
    }

    @Test
    public void testProcessMediaButton_Previous() {
        setUpService();

        mService.processAction(
                createMediaButtonActionIntent(KeyEvent.KEYCODE_MEDIA_PREVIOUS), getManager());
        verify(getManager()).onMediaSessionAction(MediaSessionAction.PREVIOUS_TRACK);
    }

    @Test
    public void testProcessMediaButton_Next() {
        setUpService();

        mService.processAction(
                createMediaButtonActionIntent(KeyEvent.KEYCODE_MEDIA_NEXT), getManager());
        verify(getManager()).onMediaSessionAction(MediaSessionAction.NEXT_TRACK);
    }

    @Test
    public void testProcessMediaButton_Rewind() {
        setUpService();

        mService.processAction(
                createMediaButtonActionIntent(KeyEvent.KEYCODE_MEDIA_FAST_FORWARD), getManager());
        verify(getManager()).onMediaSessionAction(MediaSessionAction.SEEK_FORWARD);
    }

    @Test
    public void testProcessMediaButton_Backward() {
        setUpService();

        mService.processAction(
                createMediaButtonActionIntent(KeyEvent.KEYCODE_MEDIA_REWIND), getManager());
        verify(getManager()).onMediaSessionAction(MediaSessionAction.SEEK_BACKWARD);
    }

    @Test
    public void testProcessMediaButtonActionWithNoKeyEvent() {
        setUpService();

        clearInvocations(getManager());
        mService.processAction(new Intent(Intent.ACTION_MEDIA_BUTTON), getManager());

        verifyZeroInteractions(getManager());
    }

    @Test
    public void testProcessMediaButtonActionWithWrongTypeKeyEvent() {
        setUpService();

        clearInvocations(getManager());
        mService.processAction(
                new Intent(Intent.ACTION_MEDIA_BUTTON)
                        .putExtra(Intent.EXTRA_KEY_EVENT,
                                new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_MEDIA_PLAY)),
                getManager());
        mService.processAction(new Intent(Intent.ACTION_MEDIA_BUTTON)
                                       .putExtra(Intent.EXTRA_KEY_EVENT,
                                               new KeyEvent(KeyEvent.ACTION_MULTIPLE,
                                                       KeyEvent.KEYCODE_MEDIA_PLAY)),
                getManager());

        verifyZeroInteractions(getManager());
    }

    @Test
    public void testProcessNotificationButtonAction_Stop() {
        setUpService();

        MediaNotificationManager manager = getManager();
        ListenerService service = mService;

        mService.processAction(new Intent(ListenerService.ACTION_STOP), getManager());
        verify(manager).onStop(MediaNotificationListener.ACTION_SOURCE_MEDIA_NOTIFICATION);
        verify(service).stopListenerService();
    }

    @Test
    public void testProcessNotificationButtonAction_Swipe() {
        setUpService();

        MediaNotificationManager manager = getManager();
        ListenerService service = mService;

        mService.processAction(new Intent(ListenerService.ACTION_SWIPE), getManager());
        verify(manager).onStop(MediaNotificationListener.ACTION_SOURCE_MEDIA_NOTIFICATION);
        verify(service).stopListenerService();
    }

    @Test
    public void testProcessNotificationButtonAction_Cancel() {
        setUpService();

        MediaNotificationManager manager = getManager();
        ListenerService service = mService;

        mService.processAction(new Intent(ListenerService.ACTION_CANCEL), getManager());
        verify(manager).onStop(MediaNotificationListener.ACTION_SOURCE_MEDIA_NOTIFICATION);
        verify(service).stopListenerService();
    }

    @Test
    public void testProcessNotificationButtonAction_Play() {
        setUpService();

        mService.processAction(new Intent(ListenerService.ACTION_PLAY), getManager());
        verify(getManager()).onPlay(MediaNotificationListener.ACTION_SOURCE_MEDIA_NOTIFICATION);
    }

    @Test
    public void testProcessNotificationButtonAction_Pause() {
        setUpService();

        mService.processAction(new Intent(ListenerService.ACTION_PAUSE), getManager());
        verify(getManager()).onPause(MediaNotificationListener.ACTION_SOURCE_MEDIA_NOTIFICATION);
    }

    @Test
    public void testProcessNotificationButtonAction_Noisy() {
        setUpService();

        mService.processAction(new Intent(AudioManager.ACTION_AUDIO_BECOMING_NOISY), getManager());
        verify(getManager()).onPause(MediaNotificationListener.ACTION_SOURCE_HEADSET_UNPLUG);
    }

    @Test
    public void testProcessNotificationButtonAction_PreviousTrack() {
        setUpService();

        mService.processAction(new Intent(ListenerService.ACTION_PREVIOUS_TRACK), getManager());
        verify(getManager()).onMediaSessionAction(MediaSessionAction.PREVIOUS_TRACK);
    }

    @Test
    public void testProcessNotificationButtonAction_NextTrack() {
        setUpService();

        mService.processAction(new Intent(ListenerService.ACTION_NEXT_TRACK), getManager());
        verify(getManager()).onMediaSessionAction(MediaSessionAction.NEXT_TRACK);
    }

    @Test
    public void testProcessNotificationButtonAction_SeekForward() {
        setUpService();

        mService.processAction(new Intent(ListenerService.ACTION_SEEK_FORWARD), getManager());
        verify(getManager()).onMediaSessionAction(MediaSessionAction.SEEK_FORWARD);
    }

    @Test
    public void testProcessNotificationButtonAction_SeekBackward() {
        setUpService();

        mService.processAction(new Intent(ListenerService.ACTION_SEEK_BACKWARD), getManager());
        verify(getManager()).onMediaSessionAction(MediaSessionAction.SEEK_BACKWARD);
    }
}
