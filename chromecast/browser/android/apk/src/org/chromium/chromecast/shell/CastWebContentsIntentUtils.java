// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.content_public.browser.WebContents;

/**
 * Utils for creating and handling intents used by {@link CastWebContentsComponent} and
 * classes communicate with it.
 */
public class CastWebContentsIntentUtils {
    private static final String TAG = "cr_CastWebUtil";

    static final String ACTION_DATA_SCHEME = "cast";
    static final String ACTION_DATA_AUTHORITY = "webcontents";

    private static final boolean DEBUG = true;

    /**
     * Action type of intent from Android to cast app to notify a physical key down event.
     */
    static final String ACTION_KEY_EVENT =
            "com.google.android.apps.castshell.intent.action.KEY_EVENT";

    /**
     * Action type of intent from Android to cast app to notify the stop event of
     * CastWebContentsActivity.
     */
    static final String ACTION_ACTIVITY_STOPPED =
            "com.google.android.apps.castshell.intent.action.ACTIVITY_STOPPED";

    /**
     * Action type of intent from Android to cast app to notify the visibility change
     * of cast app in an Android app.
     */
    static final String ACTION_ON_VISIBILITY_CHANGE =
            "com.google.android.apps.castshell.intent.action.ON_VISIBILITY_CHANGE";

    /**
     * Action type of intent from Android to cast app to notify a gesture detected on screen.
     */
    static final String ACTION_ON_GESTURE =
            "com.google.android.apps.castshell.intent.action.ON_GESTURE";

    /**
     * Action type of intent from cast app to android to request for a visibility priority change.
     */
    static final String ACTION_REQUEST_VISIBILITY_PRIORITY =
            "com.google.android.apps.castshell.intent.action.REQUEST_VISIBILITY_PRIORITY";

    /**
     * Action type of intent from CastShell to external acivity to notify whether the gesture
     * has been consumed.
     */
    static final String ACTION_GESTURE_CONSUMED =
            "com.google.android.apps.castshell.intent.action.GESTURE_CONSUMED";

    /**
     * Action type of intent from cast app to android to request to move the cast view out of
     * screen.
     */
    static final String ACTION_REQUEST_MOVE_OUT =
            "com.google.android.apps.castshell.intent.action.REQUEST_MOVE_OUT";

    /**
     * Action type of intent from CastWebContentsComponent to notify CastWebContentsActivity that
     * touch should be enabled.
     */
    public static final String ACTION_ENABLE_TOUCH_INPUT =
            "com.google.android.apps.castshell.intent.action.ENABLE_TOUCH_INPUT";

    /** Key of extra value in an intent, the value is a URI of cast://webcontents/<instanceId> */
    static final String INTENT_EXTRA_URI = "content_uri";

    /** Key of extra value of the intent to start a web content, value is app ID of cast app */
    static final String INTENT_EXTRA_APP_ID = "content_app_id";

    /** Key of extra value of the intent to start a web content, value is object of WebContents */
    static final String INTENT_EXTRA_WEB_CONTENTS =
            "com.google.android.apps.castshell.intent.extra.WEB_CONTENTS";

    /** Key of extra value of the intent to start a web content, value is true is touch is enabled.
     */
    static final String INTENT_EXTRA_TOUCH_INPUT_ENABLED =
            "com.google.android.apps.castshell.intent.extra.ENABLE_TOUCH";
    /**
     * Key of extra value of the intent ACTION_REQUEST_VISIBILITY, value is visibility priority
     * (int).
     */
    static final String INTENT_EXTRA_VISIBILITY_PRIORITY =
            "com.google.android.apps.castshell.intent.extra.content_visibility_priority";

    /** Key of extra value of the intent to start a web content, value is true is touch is enabled.
     */
    private static final String INTENT_EXTRA_KEY_CODE =
            "com.google.android.apps.castshell.intent.extra.KEY_CODE";

    /**
     * Key of extra value of the intent ACTION_ON_VISIBILITY_CHANGE, value is visibility type
     * (int).
     */
    private static final String INTENT_EXTRA_VISIBILITY_TYPE =
            "com.google.android.apps.castshell.intent.extra.VISIBILITY_TYPE";

    /**
     * Key of extra value of the intent ACTION_ON_GESTURE and ACTION_GUESTURE_CONSUMED, value
     * is gesture type (int).
     */
    private static final String INTENT_EXTRA_GESTURE_TYPE =
            "com.google.android.apps.castshell.intent.extra.GESTURE_TYPE";

    /**
     * Key of extra value of the intent ACTION_GUESTURE_CONSUMED, value is whether gesture
     * is consumed(consumed: true, not: false).
     */
    private static final String INTENT_EXTRA_GESTURE_CONSUMED =
            "com.google.android.apps.castshell.intent.extra.GESTURE_CONSUMED";

    // Matches to chromecast::shell::VisibilityPriority
    static final int VISIBITY_TYPE_UNKNOWN = 0;
    static final int VISIBITY_TYPE_FULL_SCREEN = 1;
    static final int VISIBITY_TYPE_PARTIAL_OUT = 2;
    static final int VISIBITY_TYPE_HIDDEN = 3;

    // CastWebContentsSurfaceHelper -> CastWebContentsComponent.Receiver
    // -> CastContentWindowAndroid
    public static Intent onActivityStopped(String instanceId) {
        Intent intent = new Intent(ACTION_ACTIVITY_STOPPED, getInstanceUri(instanceId));
        return intent;
    }

    // Host activity of CastWebContentsFragment -> CastWebContentsComponent.Receiver
    // -> CastContentWindowAndroid
    public static Intent onGesture(String instanceId, int gestureType) {
        return onGesture(getInstanceUri(instanceId), gestureType);
    }

    // Host activity of CastWebContentsFragment -> CastWebContentsComponent.Receiver
    // -> CastContentWindowAndroid
    public static Intent onGestureWithUriString(String uri, int gestureType) {
        return onGesture(Uri.parse(uri), gestureType);
    }

    private static Intent onGesture(Uri uri, int gestureType) {
        if (DEBUG) Log.d(TAG, "onGesture with uri:" + uri + " type:" + gestureType);
        Intent intent = new Intent(ACTION_ON_GESTURE, uri);
        intent.putExtra(INTENT_EXTRA_GESTURE_TYPE, gestureType);
        return intent;
    }

    // CastWebContentsActivity -> CastWebContentsComponent.Receiver
    // -> CastContentWindowAndroid
    public static Intent onKeyDown(String instanceId, int keyCode) {
        Intent intent = new Intent(ACTION_KEY_EVENT, getInstanceUri(instanceId));
        intent.putExtra(INTENT_EXTRA_KEY_CODE, keyCode);
        return intent;
    }

    // Host activity of CastWebContentsFragment -> CastWebContentsComponent.Receiver
    // -> CastContentWindowAndroid
    public static Intent onVisibilityChange(String instanceId, int visibilityType) {
        return onVisibilityChange(getInstanceUri(instanceId), visibilityType);
    }

    // Host activity of CastWebContentsFragment -> CastWebContentsComponent.Receiver
    // -> CastContentWindowAndroid
    public static Intent onVisibilityChangeWithUriString(String uri, int visibilityType) {
        return onVisibilityChange(Uri.parse(uri), visibilityType);
    }

    private static Intent onVisibilityChange(Uri uri, int visibilityType) {
        if (DEBUG) Log.d(TAG, "onVisibilityChange with uri:" + uri + " type:" + visibilityType);

        Intent intent = new Intent(ACTION_ON_VISIBILITY_CHANGE, uri);
        intent.putExtra(INTENT_EXTRA_VISIBILITY_TYPE, visibilityType);
        return intent;
    }

    // CastContentWindowAndroid -> Host activity of CastWebContentsFragment
    public static Intent requestMoveOut(String instanceId) {
        Intent intent = new Intent(ACTION_REQUEST_MOVE_OUT);
        intent.putExtra(INTENT_EXTRA_URI, getInstanceUri(instanceId).toString());
        return intent;
    }

    // CastContentWindowAndroid -> Host activity of CastWebContentsFragment
    public static Intent requestVisibilityPriority(String instanceId, int visibilityPriority) {
        Intent intent = new Intent(ACTION_REQUEST_VISIBILITY_PRIORITY);
        intent.putExtra(INTENT_EXTRA_URI, getInstanceUri(instanceId).toString());
        intent.putExtra(INTENT_EXTRA_VISIBILITY_PRIORITY, visibilityPriority);
        return intent;
    }

    // CastWebContentsComponent.Receiver -> Host activity of CastWebContentsFragment
    public static Intent gestureConsumed(String instanceId, int gestureType, boolean consumed) {
        Intent intent = new Intent(ACTION_GESTURE_CONSUMED);
        intent.putExtra(INTENT_EXTRA_URI, getInstanceUri(instanceId).toString());
        intent.putExtra(INTENT_EXTRA_GESTURE_TYPE, gestureType);
        intent.putExtra(INTENT_EXTRA_GESTURE_CONSUMED, consumed);
        return intent;
    }

    // Used by intent of ACTION_ON_GESTURE
    public static int getGestureType(Intent in) {
        return in.getIntExtra(INTENT_EXTRA_GESTURE_TYPE, 0);
    }

    // Used by intent of ACTION_KEY_EVENT
    public static int getKeyCode(Intent in) {
        return in.getIntExtra(INTENT_EXTRA_KEY_CODE, 0);
    }

    // Used by intent of ACTION_ON_VISIBILITY_CHANGE
    public static int getVisibilityType(Intent in) {
        return in.getIntExtra(INTENT_EXTRA_VISIBILITY_TYPE, 0);
    }

    // Used by intent of ACTION_KEY_EVENT
    public static int getVisibilityPriority(Bundle bundle) {
        return bundle.getInt(INTENT_EXTRA_VISIBILITY_PRIORITY, 0);
    }

    // Used by intent of ACTION_REQUEST_VISIBILITY_PRIORITY, ACTION_SHOW_WEB_CONTENT
    public static int getVisibilityPriority(Intent in) {
        return getVisibilityPriority(in.getExtras());
    }

    // Used by intent of ACTION_REQUEST_VISIBILITY_PRIORITY, ACTION_SHOW_WEB_CONTENT
    public static boolean isGestureConsumed(Intent in) {
        return isGestureConsumed(in.getExtras());
    }

    // Used by intent of ACTION_REQUEST_VISIBILITY_PRIORITY, ACTION_SHOW_WEB_CONTENT
    public static boolean isGestureConsumed(Bundle bundle) {
        return bundle.getBoolean(INTENT_EXTRA_GESTURE_CONSUMED);
    }

    public static boolean isIntentOfActivityStopped(Intent in) {
        return in.getAction().equals(ACTION_ACTIVITY_STOPPED);
    }

    public static boolean isIntentOfGesturing(Intent in) {
        return in.getAction().equals(ACTION_ON_GESTURE);
    }

    public static boolean isIntentOfKeyEvent(Intent in) {
        return in.getAction().equals(ACTION_KEY_EVENT);
    }

    public static boolean isIntentOfVisibilityChange(Intent in) {
        return in.getAction().equals(ACTION_ON_VISIBILITY_CHANGE);
    }

    public static boolean isIntentToRequestVisibilityPriority(Intent in) {
        return in.getAction().equals(ACTION_REQUEST_VISIBILITY_PRIORITY);
    }

    public static boolean isIntentToRequestMoveOut(Intent in) {
        return in.getAction().equals(ACTION_REQUEST_MOVE_OUT);
    }

    // CastWebContentsComponent.Receiver -> CastWebContentsActivity
    public static Intent requestStartCastActivity(
            Context context, WebContents webContents, boolean enableTouch, String instanceId) {
        Intent intent =
                new Intent(Intent.ACTION_VIEW, null, context, CastWebContentsActivity.class);
        intent.putExtra(INTENT_EXTRA_URI, getInstanceUri(instanceId).toString());
        intent.putExtra(INTENT_EXTRA_WEB_CONTENTS, webContents);
        intent.putExtra(INTENT_EXTRA_TOUCH_INPUT_ENABLED, enableTouch);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP
                | Intent.FLAG_ACTIVITY_NO_ANIMATION);
        return intent;
    }

    // CastWebContentsComponent.Receiver -> Host activity of CastWebContentsFragment
    public static Intent requestStartCastFragment(WebContents webContents, String appId,
            int visibilityPriority, boolean enableTouch, String instanceId) {
        Intent intent = new Intent();
        intent.setAction(CastIntents.ACTION_SHOW_WEB_CONTENT);
        intent.putExtra(INTENT_EXTRA_URI, getInstanceUri(instanceId).toString());
        intent.putExtra(INTENT_EXTRA_APP_ID, appId);
        intent.putExtra(INTENT_EXTRA_VISIBILITY_PRIORITY, visibilityPriority);
        intent.putExtra(INTENT_EXTRA_TOUCH_INPUT_ENABLED, enableTouch);
        intent.putExtra(INTENT_EXTRA_WEB_CONTENTS, webContents);
        return intent;
    }

    // CastWebContentsComponent.Receiver -> CastWebContentsService
    public static Intent requestStartCastService(
            Context context, WebContents webContents, String instanceId) {
        Intent intent = new Intent(Intent.ACTION_VIEW, getInstanceUri(instanceId), context,
                CastWebContentsService.class);
        intent.putExtra(INTENT_EXTRA_WEB_CONTENTS, webContents);
        return intent;
    }

    // CastWebContentsComponent.Delegate -> CastWebContentsSurfaceHelper
    public static Intent requestStopWebContents(String instanceId) {
        Intent intent = new Intent(CastIntents.ACTION_STOP_WEB_CONTENT);
        intent.putExtra(INTENT_EXTRA_URI, getInstanceUri(instanceId).toString());
        return intent;
    }

    // Used by ACTION_SHOW_WEB_CONTENT
    public static String getAppId(Bundle bundle) {
        return bundle.getString(INTENT_EXTRA_APP_ID);
    }

    // Used by ACTION_SHOW_WEB_CONTENT
    public static String getAppId(Intent in) {
        return getAppId(in.getExtras());
    }

    // Used by ACTION_VIEW, ACTION_SHOW_WEB_CONTENT
    public static WebContents getWebContents(Bundle bundle) {
        return (WebContents) bundle.getParcelable(INTENT_EXTRA_WEB_CONTENTS);
    }

    // Used by ACTION_VIEW, ACTION_SHOW_WEB_CONTENT
    public static WebContents getWebContents(Intent in) {
        return getWebContents(in.getExtras());
    }

    // Used by ACTION_VIEW, ACTION_SHOW_WEB_CONTENT
    public static String getUriString(Bundle bundle) {
        return bundle.getString(INTENT_EXTRA_URI);
    }

    // Used by ACTION_VIEW, ACTION_SHOW_WEB_CONTENT
    public static String getUriString(Intent in) {
        return getUriString(in.getExtras());
    }

    // Used by ACTION_VIEW, ACTION_SHOW_WEB_CONTENT
    public static boolean isTouchable(Bundle bundle) {
        return bundle.getBoolean(INTENT_EXTRA_TOUCH_INPUT_ENABLED);
    }

    // Used by ACTION_VIEW, ACTION_SHOW_WEB_CONTENT
    public static boolean isTouchable(Intent in) {
        return isTouchable(in.getExtras());
    }

    // CastWebContentsComponent -> CastWebContentsSurfaceHelper and host activity of
    // CastWebContentsFragment
    public static Intent enableTouchInput(String instanceId, boolean enabled) {
        Intent intent = new Intent(ACTION_ENABLE_TOUCH_INPUT);
        intent.putExtra(INTENT_EXTRA_URI, getInstanceUri(instanceId).toString());
        intent.putExtra(INTENT_EXTRA_TOUCH_INPUT_ENABLED, enabled);
        return intent;
    }

    // CastWebContentsSurfaceHelper -> CastWebContentsActivity or host activity
    // of CastWebContentsFragment
    public static Intent onWebContentStopped(Uri uri) {
        Intent intent = new Intent(CastIntents.ACTION_ON_WEB_CONTENT_STOPPED);
        intent.putExtra(INTENT_EXTRA_URI, uri.toString());
        return intent;
    }

    public static Uri getInstanceUri(String instanceId) {
        Uri instanceUri = new Uri.Builder()
                                  .scheme(ACTION_DATA_SCHEME)
                                  .authority(ACTION_DATA_AUTHORITY)
                                  .path(instanceId)
                                  .build();
        return instanceUri;
    }

    static LocalBroadcastManager getLocalBroadcastManager() {
        return LocalBroadcastManager.getInstance(ContextUtils.getApplicationContext());
    }
}
