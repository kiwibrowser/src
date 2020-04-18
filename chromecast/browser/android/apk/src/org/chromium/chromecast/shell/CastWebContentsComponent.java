// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.IBinder;
import android.os.PatternMatcher;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chromecast.base.Controller;
import org.chromium.content_public.browser.WebContents;

/**
 * A layer of indirection between CastContentWindowAndroid and CastWebContents(Activity|Service).
 * <p>
 * On builds with DISPLAY_WEB_CONTENTS_IN_SERVICE set to false, it will use CastWebContentsActivity,
 * otherwise, it will use CastWebContentsService.
 */
public class CastWebContentsComponent {
    /**
     * Callback interface for when the associated component is closed or the
     * WebContents is detached.
     */
    public interface OnComponentClosedHandler { void onComponentClosed(); }

    /**
     * Callback interface for passing along keyDown events. This only applies
     * to CastWebContentsActivity, really.
     */
    public interface OnKeyDownHandler { void onKeyDown(int keyCode); }

    /**
     * Callback interface for when UI events occur.
     */
    public interface SurfaceEventHandler {
        void onVisibilityChange(int visibilityType);
        boolean consumeGesture(int gestureType);
    }

    /**
     * Params to start WebContents in activity, fragment or service.
     */
    static class StartParams {
        public final Context context;
        public final WebContents webContents;
        public final String appId;
        public final int visibilityPriority;

        public StartParams(Context context, WebContents webContents, String id, int priority) {
            this.context = context;
            this.webContents = webContents;
            appId = id;
            visibilityPriority = priority;
        }
    }

    @VisibleForTesting
    interface Delegate {
        void start(StartParams params);
        void stop(Context context);
    }

    @VisibleForTesting
    class ActivityDelegate implements Delegate {
        private static final String TAG = "cr_CastWebContent_AD";
        private boolean mStarted = false;

        public ActivityDelegate(boolean enableTouchInput) {
            mEnableTouchInput = enableTouchInput;
        }

        @Override
        public void start(StartParams params) {
            if (mStarted) return; // No-op if already started.
            if (DEBUG) Log.d(TAG, "start: SHOW_WEB_CONTENT in activity");
            startCastActivity(params.context, params.webContents, mEnableTouchInput);
            mStarted = true;
        }

        @Override
        public void stop(Context context) {
            sendStopWebContentEvent();
            mStarted = false;
        }
    }

    private class FragmentDelegate implements Delegate {
        private static final String TAG = "cr_CastWebContent_FD";

        public FragmentDelegate(boolean enableTouchInput) {
            mEnableTouchInput = enableTouchInput;
        }

        @Override
        public void start(StartParams params) {
            if (!sendIntent(CastWebContentsIntentUtils.requestStartCastFragment(params.webContents,
                        params.appId, params.visibilityPriority, mEnableTouchInput, mInstanceId))) {
                // No intent receiver to handle SHOW_WEB_CONTENT in fragment
                startCastActivity(params.context, params.webContents, mEnableTouchInput);
            }
        }

        @Override
        public void stop(Context context) {
            sendStopWebContentEvent();
        }
    }

    private void startCastActivity(Context context, WebContents webContents, boolean enableTouch) {
        Intent intent = CastWebContentsIntentUtils.requestStartCastActivity(
                context, webContents, enableTouch, mInstanceId);
        if (DEBUG) Log.d(TAG, "start activity by intent: " + intent);
        context.startActivity(intent);
    }

    private void sendStopWebContentEvent() {
        Intent intent = CastWebContentsIntentUtils.requestStopWebContents(mInstanceId);
        if (DEBUG) Log.d(TAG, "stop: send STOP_WEB_CONTENT intent: " + intent);
        sendIntentSync(intent);
    }

    private class ServiceDelegate implements Delegate {
        private static final String TAG = "cr_CastWebContent_SD";

        private ServiceConnection mConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {}

            @Override
            public void onServiceDisconnected(ComponentName name) {
                if (DEBUG) Log.d(TAG, "onServiceDisconnected");

                if (mComponentClosedHandler != null) mComponentClosedHandler.onComponentClosed();
            }
        };

        @Override
        public void start(StartParams params) {
            if (DEBUG) Log.d(TAG, "start");
            Intent intent = CastWebContentsIntentUtils.requestStartCastService(
                    params.context, params.webContents, mInstanceId);
            params.context.bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
        }

        @Override
        public void stop(Context context) {
            if (DEBUG) Log.d(TAG, "stop");
            context.unbindService(mConnection);
        }
    }

    private static final String TAG = "cr_CastWebComponent";
    private static final boolean DEBUG = true;

    private final OnComponentClosedHandler mComponentClosedHandler;
    private final OnKeyDownHandler mKeyDownHandler;
    private final String mInstanceId;
    private final SurfaceEventHandler mSurfaceEventHandler;
    private final Controller<WebContents> mHasWebContentsState = new Controller<>();
    private Delegate mDelegate;
    private boolean mStarted;
    private boolean mEnableTouchInput;

    public CastWebContentsComponent(String instanceId,
            OnComponentClosedHandler onComponentClosedHandler, OnKeyDownHandler onKeyDownHandler,
            SurfaceEventHandler surfaceEventHandler, boolean isHeadless, boolean enableTouchInput) {
        if (DEBUG) {
            Log.d(TAG,
                    "New CastWebContentsComponent. Instance ID: " + instanceId + "; isHeadless: "
                            + isHeadless + "; enableTouchInput:" + enableTouchInput);
        }
        mComponentClosedHandler = onComponentClosedHandler;
        mKeyDownHandler = onKeyDownHandler;
        mInstanceId = instanceId;
        mSurfaceEventHandler = surfaceEventHandler;
        if (BuildConfig.DISPLAY_WEB_CONTENTS_IN_SERVICE || isHeadless) {
            if (DEBUG) Log.d(TAG, "Creating service delegate...");
            mDelegate = new ServiceDelegate();
        } else if (BuildConfig.ENABLE_CAST_FRAGMENT) {
            if (DEBUG) Log.d(TAG, "Creating fragment delegate...");
            mDelegate = new FragmentDelegate(enableTouchInput);
        } else {
            if (DEBUG) Log.d(TAG, "Creating activity delegate...");
            mDelegate = new ActivityDelegate(enableTouchInput);
        }

        mHasWebContentsState.watch(() -> {
            final IntentFilter filter = new IntentFilter();
            Uri instanceUri = CastWebContentsIntentUtils.getInstanceUri(instanceId);
            filter.addDataScheme(instanceUri.getScheme());
            filter.addDataAuthority(instanceUri.getAuthority(), null);
            filter.addDataPath(instanceUri.getPath(), PatternMatcher.PATTERN_LITERAL);
            filter.addAction(CastWebContentsIntentUtils.ACTION_ACTIVITY_STOPPED);
            filter.addAction(CastWebContentsIntentUtils.ACTION_KEY_EVENT);
            filter.addAction(CastWebContentsIntentUtils.ACTION_ON_VISIBILITY_CHANGE);
            filter.addAction(CastWebContentsIntentUtils.ACTION_ON_GESTURE);
            return new LocalBroadcastReceiverScope(filter, this ::onReceiveIntent);
        });
    }

    private void onReceiveIntent(Intent intent) {
        if (CastWebContentsIntentUtils.isIntentOfActivityStopped(intent)) {
            if (DEBUG) Log.d(TAG, "onReceive ACTION_ACTIVITY_STOPPED instance=" + mInstanceId);
            if (mComponentClosedHandler != null) mComponentClosedHandler.onComponentClosed();
        } else if (CastWebContentsIntentUtils.isIntentOfKeyEvent(intent)) {
            if (DEBUG) Log.d(TAG, "onReceive ACTION_KEY_EVENT instance=" + mInstanceId);
            int keyCode = CastWebContentsIntentUtils.getKeyCode(intent);
            if (mKeyDownHandler != null) mKeyDownHandler.onKeyDown(keyCode);
        } else if (CastWebContentsIntentUtils.isIntentOfVisibilityChange(intent)) {
            int visibilityType = CastWebContentsIntentUtils.getVisibilityType(intent);
            if (DEBUG) {
                Log.d(TAG,
                        "onReceive ACTION_ON_VISIBILITY_CHANGE instance=" + mInstanceId
                                + "; visibilityType=" + visibilityType);
            }
            if (mSurfaceEventHandler != null) {
                mSurfaceEventHandler.onVisibilityChange(visibilityType);
            }
        } else if (CastWebContentsIntentUtils.isIntentOfGesturing(intent)) {
            int gestureType = CastWebContentsIntentUtils.getGestureType(intent);
            if (DEBUG) {
                Log.d(TAG,
                        "onReceive ACTION_ON_GESTURE_CHANGE instance=" + mInstanceId
                                + "; gesture=" + gestureType);
            }
            if (mSurfaceEventHandler != null) {
                if (mSurfaceEventHandler.consumeGesture(gestureType)) {
                    if (DEBUG) Log.d(TAG, "send gesture consumed instance=" + mInstanceId);
                    sendIntentSync(CastWebContentsIntentUtils.gestureConsumed(
                            mInstanceId, gestureType, true));
                } else {
                    if (DEBUG) Log.d(TAG, "send gesture NOT consumed instance=" + mInstanceId);
                    sendIntentSync(CastWebContentsIntentUtils.gestureConsumed(
                            mInstanceId, gestureType, false));
                }
            } else {
                sendIntentSync(CastWebContentsIntentUtils.gestureConsumed(
                        mInstanceId, gestureType, false));
            }
        }
    }

    @VisibleForTesting
    boolean isStarted() {
        return mStarted;
    }

    @VisibleForTesting
    void setDelegate(Delegate delegate) {
        mDelegate = delegate;
    }

    public void start(StartParams params) {
        if (DEBUG) {
            Log.d(TAG,
                    "Starting WebContents with delegate: " + mDelegate.getClass().getSimpleName()
                            + "; Instance ID: " + mInstanceId + "; App ID: " + params.appId
                            + "; Visibility Priority: " + params.visibilityPriority);
        }
        mHasWebContentsState.set(params.webContents);
        mDelegate.start(params);
        mStarted = true;
    }

    public void stop(Context context) {
        if (DEBUG) {
            Log.d(TAG,
                    "stop with delegate: " + mDelegate.getClass().getSimpleName()
                            + "; Instance ID: " + mInstanceId);
        }
        if (mStarted) {
            mHasWebContentsState.reset();
            if (DEBUG) Log.d(TAG, "Call delegate to stop");
            mDelegate.stop(context);
            mStarted = false;
        }
    }

    public void requestVisibilityPriority(int visibilityPriority) {
        if (DEBUG) Log.d(TAG, "requestVisibilityPriority: " + mInstanceId + "; Visibility:"
                + visibilityPriority);
        sendIntentSync(CastWebContentsIntentUtils.requestVisibilityPriority(
                mInstanceId, visibilityPriority));
    }

    public void requestMoveOut() {
        if (DEBUG) Log.d(TAG, "requestMoveOut: " + mInstanceId);
        sendIntentSync(CastWebContentsIntentUtils.requestMoveOut(mInstanceId));
    }

    public void enableTouchInput(boolean enabled) {
        if (DEBUG) Log.d(TAG, "enableTouchInput enabled:" + enabled);
        mEnableTouchInput = enabled;
        sendIntentSync(CastWebContentsIntentUtils.enableTouchInput(mInstanceId, enabled));
    }

    public static void onComponentClosed(String instanceId) {
        if (DEBUG) Log.d(TAG, "onComponentClosed");
        sendIntentSync(CastWebContentsIntentUtils.onActivityStopped(instanceId));
    }

    public static void onKeyDown(String instanceId, int keyCode) {
        if (DEBUG) Log.d(TAG, "onKeyDown");
        sendIntentSync(CastWebContentsIntentUtils.onKeyDown(instanceId, keyCode));
    }

    public static void onVisibilityChange(String instanceId, int visibilityType) {
        if (DEBUG) Log.d(TAG, "onVisibilityChange");
        sendIntentSync(CastWebContentsIntentUtils.onVisibilityChange(instanceId, visibilityType));
    }

    public static void onGesture(String instanceId, int gestureType) {
        if (DEBUG) Log.d(TAG, "onGesture: " + instanceId + "; gestureType: "+ gestureType);
        sendIntentSync(CastWebContentsIntentUtils.onGesture(instanceId, gestureType));
    }

    private static boolean sendIntent(Intent in) {
        return CastWebContentsIntentUtils.getLocalBroadcastManager().sendBroadcast(in);
    }

    private static void sendIntentSync(Intent in) {
        CastWebContentsIntentUtils.getLocalBroadcastManager().sendBroadcastSync(in);
    }
}
