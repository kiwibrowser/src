// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.app.Activity;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;

import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.RemovableInRelease;
import org.chromium.chromecast.base.Both;
import org.chromium.chromecast.base.Consumer;
import org.chromium.chromecast.base.Controller;
import org.chromium.chromecast.base.Observable;
import org.chromium.chromecast.base.ScopeFactories;
import org.chromium.chromecast.base.ScopeFactory;
import org.chromium.chromecast.base.Unit;
import org.chromium.content.browser.ActivityContentVideoViewEmbedder;
import org.chromium.content.browser.MediaSessionImpl;
import org.chromium.content_public.browser.ContentVideoViewEmbedder;
import org.chromium.content_public.browser.WebContents;

/**
 * A util class for CastWebContentsActivity and CastWebContentsFragment to show
 * WebContent on its views.
 * <p>
 * This class is to help the activity or fragment class to work with CastContentWindowAndroid,
 * which will start a new instance of activity or fragment. If the CastContentWindowAndroid is
 * destroyed, CastWebContentsActivity or CastWebContentsFragment should be stopped.
 * Similarily,  CastWebContentsActivity or CastWebContentsFragment is stopped, eg.
 * CastWebContentsFragment is removed from a activity or the activity holding it
 * is destroyed, or CastWebContentsActivity is closed, CastContentWindowAndroid should be
 * notified by intent.
 */
@JNINamespace("chromecast::shell")
class CastWebContentsSurfaceHelper {
    private static final String TAG = "cr_CastWebContents";

    private static final int TEARDOWN_GRACE_PERIOD_TIMEOUT_MILLIS = 300;

    private final Controller<Unit> mCreatedState = new Controller<>();
    // Activated when we have an instance URI, from which the instance ID is derived.
    // Is (re)activated when new StartParams are received, and deactivated in onDestroy().
    private final Controller<Uri> mHasUriState = new Controller<>();
    // Activated when we have WebContents to display.
    private final Controller<WebContents> mWebContentsState = new Controller<>();

    private final Consumer<Uri> mFinishCallback;
    private final Handler mHandler;

    private String mInstanceId;
    private ContentVideoViewEmbedderSetter mContentVideoViewEmbedderSetter;
    private MediaSessionGetter mMediaSessionGetter;

    // TODO(vincentli) interrupt touch event from Fragment's root view when it's false.
    private boolean mTouchInputEnabled = false;

    public static class StartParams {
        public final Uri uri;
        public final WebContents webContents;
        public final boolean touchInputEnabled;

        public StartParams(Uri uri, WebContents webContents, boolean touchInputEnabled) {
            this.uri = uri;
            this.webContents = webContents;
            this.touchInputEnabled = touchInputEnabled;
        }

        @Override
        public boolean equals(Object other) {
            if (other instanceof StartParams) {
                StartParams that = (StartParams) other;
                return this.uri.equals(that.uri) && this.webContents.equals(that.webContents)
                        && this.touchInputEnabled == that.touchInputEnabled;
            }
            return false;
        }

        public static StartParams fromBundle(Bundle bundle) {
            final String uriString = CastWebContentsIntentUtils.getUriString(bundle);
            if (uriString == null) {
                Log.i(TAG, "Intent without uri received!");
                return null;
            }
            final Uri uri = Uri.parse(uriString);
            if (uri == null) {
                Log.i(TAG, "Invalid URI string: %s", uriString);
                return null;
            }
            bundle.setClassLoader(WebContents.class.getClassLoader());
            final WebContents webContents = CastWebContentsIntentUtils.getWebContents(bundle);
            if (webContents == null) {
                Log.e(TAG, "Received null WebContents in bundle.");
                return null;
            }

            final boolean touchInputEnabled = CastWebContentsIntentUtils.isTouchable(bundle);
            return new StartParams(uri, webContents, touchInputEnabled);
        }
    }

    /**
     * @param hostActivity Activity hosts the view showing WebContents
     * @param webContentsView A ScopeFactory that displays incoming WebContents.
     * @param finishCallback Invoked to tell host to finish.
     */
    CastWebContentsSurfaceHelper(Activity hostActivity, ScopeFactory<WebContents> webContentsView,
            Consumer<Uri> finishCallback) {
        mFinishCallback = finishCallback;
        mHandler = new Handler();
        mContentVideoViewEmbedderSetter =
                (WebContents webContents, ContentVideoViewEmbedder embedder)
                -> nativeSetContentVideoViewEmbedder(webContents, embedder);

        mMediaSessionGetter =
                (WebContents webContents) -> MediaSessionImpl.fromWebContents(webContents);

        // Receive broadcasts indicating the screen turned off while we have active WebContents.
        mHasUriState.watch((Uri uri) -> {
            IntentFilter filter = new IntentFilter();
            filter.addAction(CastIntents.ACTION_SCREEN_OFF);
            return new LocalBroadcastReceiverScope(filter, (Intent intent) -> {
                mWebContentsState.reset();
                maybeFinishLater(uri);
            });
        });

        // Receive broadcasts requesting to tear down this app while we have a valid URI.
        mHasUriState.watch((Uri uri) -> {
            IntentFilter filter = new IntentFilter();
            filter.addAction(CastIntents.ACTION_STOP_WEB_CONTENT);
            return new LocalBroadcastReceiverScope(filter, (Intent intent) -> {
                String intentUri = CastWebContentsIntentUtils.getUriString(intent);
                Log.d(TAG, "Intent action=" + intent.getAction() + "; URI=" + intentUri);
                if (!uri.toString().equals(intentUri)) {
                    Log.d(TAG, "Current URI=" + uri + "; intent URI=" + intentUri);
                    return;
                }
                mWebContentsState.reset();
                maybeFinishLater(uri);
            });
        });

        // Receive broadcasts indicating that touch input should be enabled.
        // TODO(yyzhong) Handle this intent in an external activity hosting a cast fragment as well.
        mHasUriState.watch((Uri uri) -> {
            IntentFilter filter = new IntentFilter();
            filter.addAction(CastWebContentsIntentUtils.ACTION_ENABLE_TOUCH_INPUT);
            return new LocalBroadcastReceiverScope(filter, (Intent intent) -> {
                String intentUri = CastWebContentsIntentUtils.getUriString(intent);
                Log.d(TAG, "Intent action=" + intent.getAction() + "; URI=" + intentUri);
                if (!uri.toString().equals(intentUri)) {
                    Log.d(TAG, "Current URI=" + uri + "; intent URI=" + intentUri);
                    return;
                }
                mTouchInputEnabled = CastWebContentsIntentUtils.isTouchable(intent);
            });
        });

        // webContentsView is responsible for displaying each new WebContents.
        mWebContentsState.watch(webContentsView);

        // Take audio focus when receiving new WebContents.
        mWebContentsState.map(webContents -> mMediaSessionGetter.get(webContents))
                .watch(ScopeFactories.onEnter(MediaSessionImpl::requestSystemAudioFocus));

        // Miscellaneous actions responding to WebContents lifecycle.
        mWebContentsState.watch((WebContents webContents) -> {
            // Set ContentVideoViewEmbedder to allow video playback.
            mContentVideoViewEmbedderSetter.set(
                    webContents, new ActivityContentVideoViewEmbedder(hostActivity));
            // Whenever our app is visible, volume controls should modify the music stream.
            // For more information read:
            // http://developer.android.com/training/managing-audio/volume-playback.html
            hostActivity.setVolumeControlStream(AudioManager.STREAM_MUSIC);
            // Notify CastWebContentsComponent when closed.
            return () -> CastWebContentsComponent.onComponentClosed(mInstanceId);
        });

        // When onDestroy() is called after onNewStartParams(), log and reset StartParams states.
        mHasUriState.andThen(Observable.not(mCreatedState))
                .map(Both::getFirst)
                .watch(ScopeFactories.onEnter((Uri uri) -> {
                    Log.d(TAG, "onDestroy: " + uri);
                    mWebContentsState.reset();
                    mHasUriState.reset();
                }));

        mCreatedState.set(Unit.unit());
    }

    void onNewStartParams(final StartParams params) {
        mTouchInputEnabled = params.touchInputEnabled;
        Log.d(TAG, "content_uri=" + params.uri);
        mHasUriState.set(params.uri);
        mWebContentsState.set(params.webContents);
        // Mutate instance ID cache only after observers have reacted to new web contents.
        mInstanceId = params.uri.getPath();
    }

    // Closes this activity if a new WebContents is not being displayed.
    private void maybeFinishLater(Uri uri) {
        Log.d(TAG, "maybeFinishLater: " + uri);
        final String currentInstanceId = mInstanceId;
        mHandler.postDelayed(() -> {
            if (currentInstanceId != null && currentInstanceId.equals(mInstanceId)) {
                mFinishCallback.accept(uri);
            }
        }, TEARDOWN_GRACE_PERIOD_TIMEOUT_MILLIS);
    }

    // Destroys all resources. After calling this method, this object must be dropped.
    void onDestroy() {
        mCreatedState.reset();
    }

    String getInstanceId() {
        return mInstanceId;
    }

    boolean isTouchInputEnabled() {
        return mTouchInputEnabled;
    }

    @RemovableInRelease
    void setMediaSessionGetterForTesting(MediaSessionGetter mediaSessionGetter) {
        mMediaSessionGetter = mediaSessionGetter;
    }

    interface MediaSessionGetter {
        MediaSessionImpl get(WebContents webContents);
    }

    @RemovableInRelease
    void setContentVideoViewEmbedderSetterForTesting(ContentVideoViewEmbedderSetter cvves) {
        mContentVideoViewEmbedderSetter = cvves;
    }

    interface ContentVideoViewEmbedderSetter {
        void set(WebContents webContents, ContentVideoViewEmbedder embedder);
    }

    private native void nativeSetContentVideoViewEmbedder(
            WebContents webContents, ContentVideoViewEmbedder embedder);
}
