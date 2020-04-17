// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Point;
import android.provider.Settings;
import android.view.Display;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.ContentVideoView;
import org.chromium.content_public.browser.ContentVideoViewEmbedder;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/**
 * A fullscreen view for accelerated video playback using surface view.
 */
@JNINamespace("content")
public class ContentVideoViewImpl
        extends FrameLayout implements ContentVideoView, SurfaceHolder.Callback {
    private static final String TAG = "cr_ContentVideoView";

    /**
     * Keep these error codes in sync with the code we defined in
     * MediaPlayerListener.java.
     */
    public static final int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 2;
    public static final int MEDIA_ERROR_INVALID_CODE = 3;

    private static final int STATE_ERROR = -1;
    private static final int STATE_NO_ERROR = 0;

    private SurfaceHolder mSurfaceHolder;
    private int mVideoWidth;
    private int mVideoHeight;
    private boolean mIsVideoLoaded;

    // Native pointer to C++ ContentVideoView object.
    private long mNativeContentVideoView;

    private int mCurrentState = STATE_NO_ERROR;

    // Strings for displaying media player errors
    private String mPlaybackErrorText;
    private String mUnknownErrorText;
    private String mErrorButton;
    private String mErrorTitle;

    // This view will contain the video.
    private VideoSurfaceView mVideoSurfaceView;

    private final ContentVideoViewEmbedder mEmbedder;

    private boolean mInitialOrientation;
    private boolean mPossibleAccidentalChange;
    private boolean mUmaRecorded;
    private long mOrientationChangedTime;
    private long mPlaybackStartTime;

    private class VideoSurfaceView extends SurfaceView {
        public VideoSurfaceView(Context context) {
            super(context);
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            // set the default surface view size to (1, 1) so that it won't block
            // the infobar. (0, 0) is not a valid size for surface view.
            int width = 1;
            int height = 1;
            if (mVideoWidth > 0 && mVideoHeight > 0) {
                width = getDefaultSize(mVideoWidth, widthMeasureSpec);
                height = getDefaultSize(mVideoHeight, heightMeasureSpec);
                if (mVideoWidth * height > width * mVideoHeight) {
                    height = width * mVideoHeight / mVideoWidth;
                } else if (mVideoWidth * height < width * mVideoHeight) {
                    width = height * mVideoWidth / mVideoHeight;
                }
            }
            if (mUmaRecorded) {
                // If we have never switched orientation, record the orientation
                // time.
                if (mPlaybackStartTime == mOrientationChangedTime) {
                    if (isOrientationPortrait() != mInitialOrientation) {
                        mOrientationChangedTime = System.currentTimeMillis();
                    }
                } else {
                    // if user quickly switched the orientation back and force, don't
                    // count it in UMA.
                    if (!mPossibleAccidentalChange && isOrientationPortrait() == mInitialOrientation
                            && System.currentTimeMillis() - mOrientationChangedTime < 5000) {
                        mPossibleAccidentalChange = true;
                    }
                }
            }
            setMeasuredDimension(width, height);
        }
    }

    private static final ContentVideoViewEmbedder NULL_VIDEO_EMBEDDER =
            new ContentVideoViewEmbedder() {
                @Override
                public void enterFullscreenVideo(View view, boolean isVideoLoaded) {}

                @Override
                public void fullscreenVideoLoaded() {}

                @Override
                public void exitFullscreenVideo() {}

                @Override
                public void setSystemUiVisibility(boolean enterFullscreen) {}
            };

    private final Runnable mExitFullscreenRunnable = new Runnable() {
        @Override
        public void run() {
            exitFullscreen(true);
        }
    };

    private ContentVideoViewImpl(Context context, long nativeContentVideoView,
            ContentVideoViewEmbedder embedder, int videoWidth, int videoHeight) {
        super(context);
        mNativeContentVideoView = nativeContentVideoView;
        mEmbedder = embedder != null ? embedder : NULL_VIDEO_EMBEDDER;
        mUmaRecorded = false;
        mPossibleAccidentalChange = false;
        mIsVideoLoaded = videoWidth > 0 && videoHeight > 0;
        initResources(context);
        mVideoSurfaceView = new VideoSurfaceView(context);
        showContentVideoView();
        setVisibility(View.VISIBLE);
        mEmbedder.enterFullscreenVideo(this, mIsVideoLoaded);
        onVideoSizeChanged(videoWidth, videoHeight);
    }

    private ContentVideoViewEmbedder getContentVideoViewEmbedder() {
        return mEmbedder;
    }

    private void initResources(Context context) {
        if (mPlaybackErrorText != null) return;
        mPlaybackErrorText = context.getString(
                org.chromium.content.R.string.media_player_error_text_invalid_progressive_playback);
        mUnknownErrorText =
                context.getString(org.chromium.content.R.string.media_player_error_text_unknown);
        mErrorButton = context.getString(org.chromium.content.R.string.media_player_error_button);
        mErrorTitle = context.getString(org.chromium.content.R.string.media_player_error_title);
    }

    private void showContentVideoView() {
        mVideoSurfaceView.getHolder().addCallback(this);
        addView(mVideoSurfaceView,
                new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER));
    }

    @CalledByNative
    private void onMediaPlayerError(int errorType) {
        Log.d(TAG, "OnMediaPlayerError: %d", errorType);
        if (mCurrentState == STATE_ERROR) {
            return;
        }

        // Ignore some invalid error codes.
        if (errorType == MEDIA_ERROR_INVALID_CODE) {
            return;
        }

        mCurrentState = STATE_ERROR;

        if (WindowAndroid.activityFromContext(getContext()) == null) {
            Log.w(TAG, "Unable to show alert dialog because it requires an activity context");
            return;
        }

        /* Pop up an error dialog so the user knows that
         * something bad has happened. Only try and pop up the dialog
         * if we're attached to a window. When we're going away and no
         * longer have a window, don't bother showing the user an error.
         *
         * TODO(qinmin): We need to review whether this Dialog is OK with
         * the rest of the browser UI elements.
         */
        if (getWindowToken() != null) {
            String message;

            if (errorType == MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK) {
                message = mPlaybackErrorText;
            } else {
                message = mUnknownErrorText;
            }

            try {
                new AlertDialog.Builder(getContext())
                        .setTitle(mErrorTitle)
                        .setMessage(message)
                        .setPositiveButton(mErrorButton,
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int whichButton) {}
                                })
                        .setCancelable(false)
                        .show();
            } catch (RuntimeException e) {
                Log.e(TAG, "Cannot show the alert dialog, error message: %s", message, e);
            }
        }
    }

    @CalledByNative
    private void onVideoSizeChanged(int width, int height) {
        mVideoWidth = width;
        mVideoHeight = height;

        // We take non-zero frame size as an indication that the video has been loaded.
        if (!mIsVideoLoaded && mVideoWidth > 0 && mVideoHeight > 0) {
            mIsVideoLoaded = true;
            mEmbedder.fullscreenVideoLoaded();
        }

        // This will trigger the SurfaceView.onMeasure() call.
        mVideoSurfaceView.getHolder().setFixedSize(mVideoWidth, mVideoHeight);

        if (mUmaRecorded) return;

        try {
            if (Settings.System.getInt(
                        getContext().getContentResolver(), Settings.System.ACCELEROMETER_ROTATION)
                    == 0) {
                return;
            }
        } catch (Settings.SettingNotFoundException e) {
            return;
        }
        mInitialOrientation = isOrientationPortrait();
        mUmaRecorded = true;
        mPlaybackStartTime = System.currentTimeMillis();
        mOrientationChangedTime = mPlaybackStartTime;
        nativeRecordFullscreenPlayback(
                mNativeContentVideoView, mVideoHeight > mVideoWidth, mInitialOrientation);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        mSurfaceHolder = holder;
        openVideo();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (mNativeContentVideoView != 0) {
            nativeSetSurface(mNativeContentVideoView, null);
        }
        mSurfaceHolder = null;
        post(mExitFullscreenRunnable);
    }

    @CalledByNative
    private void openVideo() {
        if (mSurfaceHolder != null) {
            mCurrentState = STATE_NO_ERROR;
            if (mNativeContentVideoView != 0) {
                // Note: this may result in a reentrant call to onVideoSizeChanged().
                nativeSetSurface(mNativeContentVideoView, mSurfaceHolder.getSurface());
            }
        }
    }

    /**
     * Creates ContentVideoViewImpl. The videoWidth and videoHeight parameters designate
     * the video frame size if it is known at the time of this call, or should be 0.
     * ContentVideoViewImpl assumes that zero size means video has not been loaded yet.
     */
    @CalledByNative
    private static ContentVideoViewImpl createContentVideoView(WebContents webContents,
            ContentVideoViewEmbedder embedder, long nativeContentVideoView, int videoWidth,
            int videoHeight) {
        ThreadUtils.assertOnUiThread();
        // TODO(jinsukkim): See if the context can be obtained from WindowAndroid.
        Context context = ViewEventSinkImpl.from(webContents).getContext();
        ContentVideoViewImpl videoView = new ContentVideoViewImpl(
                context, nativeContentVideoView, embedder, videoWidth, videoHeight);
        return videoView;
    }

    private void removeSurfaceView() {
        removeView(mVideoSurfaceView);
        mVideoSurfaceView = null;
    }

    @Override
    public boolean createdWithContext(Context context) {
        return getContext() == context;
    }

    @CalledByNative
    @Override
    public void exitFullscreen(boolean releaseMediaPlayer) {
        if (mNativeContentVideoView != 0) {
            destroyContentVideoView(false);
            if (mUmaRecorded && !mPossibleAccidentalChange) {
                long currentTime = System.currentTimeMillis();
                long timeBeforeOrientationChange = mOrientationChangedTime - mPlaybackStartTime;
                long timeAfterOrientationChange = currentTime - mOrientationChangedTime;
                if (timeBeforeOrientationChange == 0) {
                    timeBeforeOrientationChange = timeAfterOrientationChange;
                    timeAfterOrientationChange = 0;
                }
                nativeRecordExitFullscreenPlayback(mNativeContentVideoView, mInitialOrientation,
                        timeBeforeOrientationChange, timeAfterOrientationChange);
            }
            nativeDidExitFullscreen(mNativeContentVideoView, releaseMediaPlayer);
            mNativeContentVideoView = 0;
        }
    }

    @Override
    public void onFullscreenWindowFocused() {
        mEmbedder.setSystemUiVisibility(true);
    }

    /**
     * This method shall only be called by native and exitFullscreen,
     * To exit fullscreen, use exitFullscreen in Java.
     */
    @CalledByNative
    private void destroyContentVideoView(boolean nativeViewDestroyed) {
        if (mVideoSurfaceView != null) {
            removeSurfaceView();
            setVisibility(View.GONE);

            // To prevent re-entrance, call this after removeSurfaceView.
            mEmbedder.exitFullscreenVideo();
        }
        if (nativeViewDestroyed) {
            mNativeContentVideoView = 0;
        }
    }

    public static ContentVideoViewImpl getInstance() {
        return nativeGetSingletonJavaContentVideoView();
    }

    private boolean isOrientationPortrait() {
        Context context = getContext();
        WindowManager manager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = manager.getDefaultDisplay();
        Point outputSize = new Point(0, 0);
        display.getSize(outputSize);
        return outputSize.x <= outputSize.y;
    }

    private static native ContentVideoViewImpl nativeGetSingletonJavaContentVideoView();
    private native void nativeDidExitFullscreen(
            long nativeContentVideoView, boolean releaseMediaPlayer);
    private native void nativeSetSurface(long nativeContentVideoView, Surface surface);
    private native void nativeRecordFullscreenPlayback(
            long nativeContentVideoView, boolean isVideoPortrait, boolean isOrientationPortrait);
    private native void nativeRecordExitFullscreenPlayback(long nativeContentVideoView,
            boolean isOrientationPortrait, long playbackDurationBeforeOrientationChange,
            long playbackDurationAfterOrientationChange);
}
