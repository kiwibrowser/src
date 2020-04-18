// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.emptyIterable;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

import android.media.AudioManager;
import android.os.Build;
import android.util.SparseIntArray;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowAudioManager;

import org.chromium.chromecast.base.Controller;
import org.chromium.chromecast.base.Observable;
import org.chromium.chromecast.base.Unit;
import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for CastAudioManager.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CastAudioManagerTest {
    @Test
    public void testAudioFocusScopeActivatedWhenRequestGranted() {
        CastAudioManager audioManager =
                CastAudioManager.getAudioManager(RuntimeEnvironment.application);
        ShadowAudioManager shadowAudioManager = Shadows.shadowOf(audioManager.getInternal());
        Controller<Unit> requestAudioFocusState = new Controller<>();
        List<String> result = new ArrayList<>();
        Observable<Unit> gotAudioFocusState = audioManager.requestAudioFocusWhen(
                requestAudioFocusState, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        gotAudioFocusState.watch(() -> {
            result.add("Got audio focus");
            return () -> result.add("Lost audio focus");
        });
        requestAudioFocusState.set(Unit.unit());
        shadowAudioManager.getLastAudioFocusRequest().listener.onAudioFocusChange(
                AudioManager.AUDIOFOCUS_GAIN);
        assertThat(result, contains("Got audio focus"));
    }

    @Test
    public void testAudioFocusScopeDeactivatedWhenFocusRequestStateIsReset() {
        CastAudioManager audioManager =
                CastAudioManager.getAudioManager(RuntimeEnvironment.application);
        ShadowAudioManager shadowAudioManager = Shadows.shadowOf(audioManager.getInternal());
        Controller<Unit> requestAudioFocusState = new Controller<>();
        List<String> result = new ArrayList<>();
        Observable<Unit> gotAudioFocusState = audioManager.requestAudioFocusWhen(
                requestAudioFocusState, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        gotAudioFocusState.watch(() -> {
            result.add("Got audio focus");
            return () -> result.add("Lost audio focus");
        });
        requestAudioFocusState.set(Unit.unit());
        shadowAudioManager.getLastAudioFocusRequest().listener.onAudioFocusChange(
                AudioManager.AUDIOFOCUS_GAIN);
        requestAudioFocusState.reset();
        assertThat(result, contains("Got audio focus", "Lost audio focus"));
    }

    @Test
    public void testAudioFocusScopeDeactivatedWhenAudioFocusIsLostButRequestStillActive() {
        CastAudioManager audioManager =
                CastAudioManager.getAudioManager(RuntimeEnvironment.application);
        ShadowAudioManager shadowAudioManager = Shadows.shadowOf(audioManager.getInternal());
        Controller<Unit> requestAudioFocusState = new Controller<>();
        List<String> result = new ArrayList<>();
        Observable<Unit> gotAudioFocusState = audioManager.requestAudioFocusWhen(
                requestAudioFocusState, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        gotAudioFocusState.watch(() -> {
            result.add("Got audio focus");
            return () -> result.add("Lost audio focus");
        });
        requestAudioFocusState.set(Unit.unit());
        AudioManager.OnAudioFocusChangeListener listener =
                shadowAudioManager.getLastAudioFocusRequest().listener;
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS);
        assertThat(result, contains("Got audio focus", "Lost audio focus"));
    }

    @Test
    public void testAudioFocusScopeReactivatedWhenAudioFocusIsLostAndRegained() {
        CastAudioManager audioManager =
                CastAudioManager.getAudioManager(RuntimeEnvironment.application);
        ShadowAudioManager shadowAudioManager = Shadows.shadowOf(audioManager.getInternal());
        Controller<Unit> requestAudioFocusState = new Controller<>();
        List<String> result = new ArrayList<>();
        Observable<Unit> gotAudioFocusState = audioManager.requestAudioFocusWhen(
                requestAudioFocusState, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        gotAudioFocusState.watch(() -> {
            result.add("Got audio focus");
            return () -> result.add("Lost audio focus");
        });
        requestAudioFocusState.set(Unit.unit());
        AudioManager.OnAudioFocusChangeListener listener =
                shadowAudioManager.getLastAudioFocusRequest().listener;
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS);
        listener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN);
        assertThat(result, contains("Got audio focus", "Lost audio focus", "Got audio focus"));
    }

    @Test
    public void testAudioFocusScopeNotActivatedIfRequestScopeNotActivated() {
        CastAudioManager audioManager =
                CastAudioManager.getAudioManager(RuntimeEnvironment.application);
        ShadowAudioManager shadowAudioManager = Shadows.shadowOf(audioManager.getInternal());
        Controller<Unit> requestAudioFocusState = new Controller<>();
        List<String> result = new ArrayList<>();
        Observable<Unit> gotAudioFocusState = audioManager.requestAudioFocusWhen(
                requestAudioFocusState, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        gotAudioFocusState.watch(() -> {
            result.add("Got audio focus");
            return () -> result.add("Lost audio focus");
        });
        assertThat(result, emptyIterable());
    }

    // Simulate the AudioManager mute behavior on Android L. The isStreamMute() method is present,
    // but can only be used through reflection. Mute requests are cumulative, so a stream only
    // unmutes once a equal number of setStreamMute(t, true) setStreamMute(t, false) requests have
    // been received.
    private static class LollipopAudioManager extends AudioManager {
        // Stores the number of total standing mute requests per stream.
        private final SparseIntArray mMuteState = new SparseIntArray();
        private boolean mCanCallStreamMute = true;

        public void setCanCallStreamMute(boolean able) {
            mCanCallStreamMute = able;
        }

        @Override
        public boolean isStreamMute(int streamType) {
            if (!mCanCallStreamMute) {
                throw new RuntimeException("isStreamMute() disabled for testing");
            }
            return mMuteState.get(streamType, 0) > 0;
        }

        @Override
        public void setStreamMute(int streamType, boolean muteState) {
            int delta = muteState ? 1 : -1;
            int currentMuteCount = mMuteState.get(streamType, 0);
            int newMuteCount = currentMuteCount + delta;
            assert newMuteCount >= 0;
            mMuteState.put(streamType, newMuteCount);
        }
    }

    @Test
    @Config(sdk = Build.VERSION_CODES.LOLLIPOP)
    public void testReleaseStreamMuteWithNoMute() {
        AudioManager fakeAudioManager = new LollipopAudioManager();
        CastAudioManager audioManager = new CastAudioManager(fakeAudioManager);
        audioManager.releaseStreamMuteIfNecessary(AudioManager.STREAM_MUSIC);
        assertFalse(fakeAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
    }

    @Test
    @Config(sdk = Build.VERSION_CODES.LOLLIPOP)
    public void testReleaseStreamMuteWithMute() {
        AudioManager fakeAudioManager = new LollipopAudioManager();
        CastAudioManager audioManager = new CastAudioManager(fakeAudioManager);
        fakeAudioManager.setStreamMute(AudioManager.STREAM_MUSIC, true);
        assertTrue(fakeAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
        audioManager.releaseStreamMuteIfNecessary(AudioManager.STREAM_MUSIC);
        assertFalse(fakeAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
    }

    @Test
    @Config(sdk = Build.VERSION_CODES.LOLLIPOP)
    public void testHandleExceptionFromIsStreamMute() {
        LollipopAudioManager fakeAudioManager = new LollipopAudioManager();
        fakeAudioManager.setCanCallStreamMute(false);
        CastAudioManager audioManager = new CastAudioManager(fakeAudioManager);
        // This should not crash even if isStreamMute() throws an exception.
        audioManager.releaseStreamMuteIfNecessary(AudioManager.STREAM_MUSIC);
    }
}
