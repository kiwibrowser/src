// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.media.AudioManager;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.media.remote.RemoteVideoInfo.PlayerState;
import org.chromium.chrome.browser.media.ui.MediaNotificationInfo;
import org.chromium.chrome.browser.media.ui.MediaNotificationListener;
import org.chromium.chrome.browser.media.ui.MediaNotificationManager;
import org.chromium.chrome.browser.metrics.MediaNotificationUma;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.content_public.common.MediaMetadata;

import java.net.URI;
import java.net.URISyntaxException;

import javax.annotation.Nullable;

/**
 * Notification control controls the cast notification and lock screen, using
 * {@link MediaNotificationManager}
 */
public class CastNotificationControl implements MediaRouteController.UiListener,
        MediaNotificationListener, AudioManager.OnAudioFocusChangeListener {
    private static final String TAG = "MediaFling";

    @SuppressLint("StaticFieldLeak")
    private static CastNotificationControl sInstance;

    private Bitmap mPosterBitmap;
    protected MediaRouteController mMediaRouteController;
    private MediaNotificationInfo.Builder mNotificationBuilder;
    private Context mContext;
    private PlayerState mState;
    private String mTitle = "";
    private AudioManager mAudioManager;

    /**
     * Contains the origin of the tab containing the video when it's cast. The origin is formatted
     * to be presented better from the security POV if the URL is parseable. Otherwise it's just the
     * URL of the tab if the tab exists. Can be null.
     */
    @Nullable
    private String mTabOrigin;

    private boolean mIsShowing;

    private static final Object LOCK = new Object();


    private CastNotificationControl(Context context) {
        mContext = context;
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
    }
    /**
     * Get the unique NotificationControl, creating it if necessary.
     * @param context The context of the activity
     * @param mediaRouteController The current mediaRouteController, if any.
     * @return a {@code LockScreenTransportControl} based on the platform's SDK API or null if the
     *         current platform's SDK API is not supported.
     */
    public static CastNotificationControl getOrCreate(Context context,
            @Nullable MediaRouteController mediaRouteController) {
        synchronized (LOCK) {
            if (sInstance == null) {
                sInstance = new CastNotificationControl(context);
            }
            sInstance.setRouteController(mediaRouteController);
            return sInstance;
        }
    }

    /**
     * @return the poster bitmap previously assigned with {@link #setPosterBitmap(Bitmap)}, or
     * {@code null} if the poster bitmap has not yet been assigned.
     */
    public final Bitmap getPosterBitmap() {
        return mPosterBitmap;
    }

    /**
     * Sets the poster bitmap to display on the TransportControl.
     */
    public final void setPosterBitmap(Bitmap posterBitmap) {
        if (mPosterBitmap == posterBitmap
                || (mPosterBitmap != null && mPosterBitmap.sameAs(posterBitmap))) {
            return;
        }
        mPosterBitmap = posterBitmap;
        if (mNotificationBuilder == null || mMediaRouteController == null) return;

        updateNotificationBuilderIfPosterIsGoodEnough();
        mNotificationBuilder.setNotificationLargeIcon(mMediaRouteController.getPoster());
        updateNotification();
    }

    public void hide() {
        mIsShowing = false;
        MediaNotificationManager.hide(Tab.INVALID_TAB_ID, R.id.remote_notification);
        mAudioManager.abandonAudioFocus(this);
        mMediaRouteController.removeUiListener(this);
    }

    public void show(PlayerState initialState) {
        mMediaRouteController.addUiListener(this);
        // TODO(aberent): investigate why this is necessary, and whether we are handling
        // it correctly. Also add code to restore it when Chrome is resumed.
        mAudioManager.requestAudioFocus(this, AudioManager.USE_DEFAULT_STREAM_TYPE,
                AudioManager.AUDIOFOCUS_GAIN);
        Intent contentIntent = new Intent(mContext, ExpandedControllerActivity.class);
        contentIntent.putExtra(MediaNotificationUma.INTENT_EXTRA_NAME,
                MediaNotificationUma.SOURCE_MEDIA_FLING);
        mNotificationBuilder = new MediaNotificationInfo.Builder()
                .setPaused(false)
                .setPrivate(false)
                .setNotificationSmallIcon(R.drawable.ic_notification_media_route)
                .setContentIntent(contentIntent)
                .setDefaultNotificationLargeIcon(R.drawable.cast_playing_square)
                .setId(R.id.remote_notification)
                .setListener(this);

        updateNotificationBuilderIfPosterIsGoodEnough();
        mState = initialState;

        updateNotification();
        mIsShowing = true;
    }

    public void setRouteController(MediaRouteController controller) {
        if (mMediaRouteController != null) mMediaRouteController.removeUiListener(this);
        mMediaRouteController = controller;
        if (controller != null) controller.addUiListener(this);
    }

    private void updateNotification() {
        // Nothing shown yet, nothing to update.
        if (mNotificationBuilder == null) return;

        mNotificationBuilder.setMetadata(new MediaMetadata(mTitle, "", ""));
        if (mTabOrigin != null) mNotificationBuilder.setOrigin(mTabOrigin);

        if (mState == PlayerState.PAUSED || mState == PlayerState.PLAYING) {
            mNotificationBuilder.setPaused(mState != PlayerState.PLAYING);
            mNotificationBuilder.setActions(MediaNotificationInfo.ACTION_STOP
                    | MediaNotificationInfo.ACTION_PLAY_PAUSE);
            MediaNotificationManager.show(mNotificationBuilder.build());
        } else if (mState == PlayerState.LOADING) {
            mNotificationBuilder.setActions(MediaNotificationInfo.ACTION_STOP);
            MediaNotificationManager.show(mNotificationBuilder.build());
        } else {
            hide();
        }
    }

    // TODO(aberent) at the moment this is only called from a test, but it should be called if the
    // poster changes.
    public void onPosterBitmapChanged() {
        if (mNotificationBuilder == null || mMediaRouteController == null) return;
        updateNotificationBuilderIfPosterIsGoodEnough();
        updateNotification();
    }

    // MediaRouteController.UiListener implementation.
    @Override
    public void onPlaybackStateChanged(PlayerState newState) {
        if (!mIsShowing
                && (newState == PlayerState.PLAYING || newState == PlayerState.LOADING
                        || newState == PlayerState.PAUSED)) {
            show(newState);
            return;
        }

        if (mState == newState
                || mState == PlayerState.PAUSED && newState == PlayerState.LOADING && mIsShowing) {
            return;
        }

        mState = newState;
        updateNotification();
    }

    @Override
    public void onRouteSelected(String name, MediaRouteController mediaRouteController) {
        // The notification will be shown/updated later so don't update it in case it's still
        // showing for the previous video.
        mTabOrigin = getCurrentTabOrigin();
    }

    @Override
    public void onRouteUnselected(MediaRouteController mediaRouteController) {
        hide();
    }

    @Override
    public void onPrepared(MediaRouteController mediaRouteController) {
    }

    @Override
    public void onError(int errorType, String message) {
    }

    @Override
    public void onDurationUpdated(long durationMillis) {
    }

    @Override
    public void onPositionChanged(long positionMillis) {
    }

    @Override
    public void onTitleChanged(String title) {
        if (title == null || title.equals(mTitle)) return;

        mTitle = title;
        updateNotification();
    }

    // MediaNotificationListener methods
    @Override
    public void onPlay(int actionSource) {
        mMediaRouteController.resume();
    }

    @Override
    public void onPause(int actionSource) {
        mMediaRouteController.pause();
    }

    @Override
    public void onStop(int actionSource) {
        mMediaRouteController.release();
    }

    @Override
    public void onMediaSessionAction(int action) {}

    // AudioManager.OnAudioFocusChangeListener methods
    @Override
    public void onAudioFocusChange(int focusChange) {
        // Do nothing. The remote player should keep playing.
    }

    @VisibleForTesting
    static CastNotificationControl getForTests() {
        return sInstance;
    }

    @VisibleForTesting
    boolean isShowingForTests() {
        return mIsShowing;
    }

    private void updateNotificationBuilderIfPosterIsGoodEnough() {
        Bitmap poster = mMediaRouteController.getPoster();
        if (MediaNotificationManager.isBitmapSuitableAsMediaImage(poster)) {
            mNotificationBuilder.setNotificationLargeIcon(poster);
            mNotificationBuilder.setMediaSessionImage(poster);
        }
    }

    private String getCurrentTabOrigin() {
        Activity activity = ApplicationStatus.getLastTrackedFocusedActivity();

        if (!(activity instanceof ChromeTabbedActivity)) return null;

        Tab tab = ((ChromeTabbedActivity) activity).getActivityTab();
        if (tab == null || !tab.isInitialized()) return null;

        String url = tab.getUrl();
        try {
            return UrlFormatter.formatUrlForSecurityDisplay(new URI(url), true);
        } catch (URISyntaxException | UnsatisfiedLinkError e) {
            // UnstatisfiedLinkError can only happen in tests as the natives are not initialized
            // yet.
            Log.e(TAG, "Unable to parse the origin from the URL. Using the full URL instead.");
            return url;
        }
    }
}
