// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;

import com.google.android.gms.cast.CastDevice;
import com.google.android.gms.cast.MediaInfo;
import com.google.android.gms.cast.MediaStatus;
import com.google.android.gms.cast.RemoteMediaPlayer;
import com.google.android.gms.common.api.GoogleApiClient;

import org.chromium.chrome.browser.media.router.MediaController;
import org.chromium.chrome.browser.media.router.cast.CastSessionUtil;
import org.chromium.chrome.browser.media.ui.MediaNotificationInfo;
import org.chromium.chrome.browser.media.ui.MediaNotificationManager;

/**
 * A wrapper around a RemoteMediaPlayer that exposes simple playback commands without the
 * the complexities of the GMS cast calls.
 */
public class RemoteMediaPlayerWrapper implements RemoteMediaPlayer.OnMetadataUpdatedListener,
                                                 RemoteMediaPlayer.OnStatusUpdatedListener,
                                                 MediaController {
    private final CastDevice mCastDevice;

    private GoogleApiClient mApiClient;
    private RemoteMediaPlayer mMediaPlayer;
    private MediaNotificationInfo.Builder mNotificationBuilder;

    public RemoteMediaPlayerWrapper(GoogleApiClient apiClient,
            MediaNotificationInfo.Builder notificationBuilder, CastDevice castDevice) {
        mApiClient = apiClient;
        mCastDevice = castDevice;
        mNotificationBuilder = notificationBuilder;

        mMediaPlayer = new RemoteMediaPlayer();
        mMediaPlayer.setOnStatusUpdatedListener(this);
        mMediaPlayer.setOnMetadataUpdatedListener(this);

        updateNotificationMetadata();
    }

    private void updateNotificationMetadata() {
        CastSessionUtil.setNotificationMetadata(mNotificationBuilder, mCastDevice, mMediaPlayer);
        MediaNotificationManager.show(mNotificationBuilder.build());
    }

    private boolean canSendCommand() {
        return mApiClient != null && mMediaPlayer != null && mApiClient.isConnected();
    }

    // RemoteMediaPlayer.OnStatusUpdatedListener implementation.
    @Override
    public void onStatusUpdated() {
        MediaStatus mediaStatus = mMediaPlayer.getMediaStatus();
        if (mediaStatus == null) return;

        int playerState = mediaStatus.getPlayerState();
        if (playerState == MediaStatus.PLAYER_STATE_PAUSED
                || playerState == MediaStatus.PLAYER_STATE_PLAYING) {
            mNotificationBuilder.setPaused(playerState != MediaStatus.PLAYER_STATE_PLAYING);
            mNotificationBuilder.setActions(
                    MediaNotificationInfo.ACTION_STOP | MediaNotificationInfo.ACTION_PLAY_PAUSE);
        } else {
            mNotificationBuilder.setActions(MediaNotificationInfo.ACTION_STOP);
        }
        MediaNotificationManager.show(mNotificationBuilder.build());
    }

    // RemoteMediaPlayer.OnMetadataUpdatedListener implementation.
    @Override
    public void onMetadataUpdated() {
        updateNotificationMetadata();
    }

    /**
     * Forwards the message to the underlying RemoteMediaPlayer.
     */
    public void onMediaMessage(String message) {
        if (mMediaPlayer != null)
            mMediaPlayer.onMessageReceived(mCastDevice, CastSessionUtil.MEDIA_NAMESPACE, message);
    }

    /**
     * Starts loading the provided media URL, without autoplay.
     */
    public void load(String mediaUrl) {
        if (!canSendCommand()) return;

        MediaInfo.Builder mediaInfoBuilder =
                new MediaInfo.Builder(mediaUrl).setContentType("*/*").setStreamType(
                        MediaInfo.STREAM_TYPE_BUFFERED);

        mMediaPlayer.load(mApiClient, mediaInfoBuilder.build(), /* autoplay */ false);
    }

    /**
     * Starts playback. No-op if are not in a valid state.
     * Doesn't verify the command's success/failure.
     */
    @Override
    public void play() {
        if (!canSendCommand()) return;

        mMediaPlayer.play(mApiClient);
    }

    /**
     * Pauses playback. No-op if are not in a valid state.
     * Doesn't verify the command's success/failure.
     */
    @Override
    public void pause() {
        if (!canSendCommand()) return;

        mMediaPlayer.pause(mApiClient);
    }

    /**
     * Sets the mute state. Does not affect the stream volume.
     * No-op if are not in a valid state. Doesn't verify the command's success/failure.
     */
    @Override
    public void setMute(boolean mute) {
        if (!canSendCommand()) return;

        mMediaPlayer.setStreamMute(mApiClient, mute);
    }

    /**
     * Sets the stream volume. Does not affect the mute state.
     * No-op if are not in a valid state. Doesn't verify the command's success/failure.
     */
    @Override
    public void setVolume(double volume) {
        if (!canSendCommand()) return;

        mMediaPlayer.setStreamVolume(mApiClient, volume);
    }

    /**
     * Seeks to the given position (in milliseconds).
     * No-op if are not in a valid state. Doesn't verify the command's success/failure.
     */
    @Override
    public void seek(long position) {
        if (!canSendCommand()) return;

        mMediaPlayer.seek(mApiClient, position);
    }

    /**
     * Called when the session has stopped, and we should no longer send commands.
     */
    public void clearApiClient() {
        mApiClient = null;
    }
}
