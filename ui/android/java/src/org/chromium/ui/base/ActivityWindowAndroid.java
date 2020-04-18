// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender.SendIntentException;
import android.view.View;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.Callback;
import org.chromium.ui.UiUtils;

import java.lang.ref.WeakReference;

/**
 * The class provides the WindowAndroid's implementation which requires
 * Activity Instance.
 * Only instantiate this class when you need the implemented features.
 */
public class ActivityWindowAndroid
        extends WindowAndroid
        implements ApplicationStatus.ActivityStateListener, View.OnLayoutChangeListener {
    // Constants used for intent request code bounding.
    private static final int REQUEST_CODE_PREFIX = 1000;
    private static final int REQUEST_CODE_RANGE_SIZE = 100;

    private int mNextRequestCode;

    /**
     * Creates an Activity-specific WindowAndroid with associated intent functionality.
     * TODO(jdduke): Remove this overload when all callsites have been updated to
     * indicate their activity state listening preference.
     * @param context Context wrapping an activity associated with the WindowAndroid.
     */
    public ActivityWindowAndroid(Context context) {
        this(context, true);
    }

    /**
     * Creates an Activity-specific WindowAndroid with associated intent functionality.
     * @param context Context wrapping an activity associated with the WindowAndroid.
     * @param listenToActivityState Whether to listen to activity state changes.
     */
    public ActivityWindowAndroid(Context context, boolean listenToActivityState) {
        super(context);
        Activity activity = activityFromContext(context);
        if (activity == null) {
            throw new IllegalArgumentException("Context is not and does not wrap an Activity");
        }
        if (listenToActivityState) {
            ApplicationStatus.registerStateListenerForActivity(this, activity);
        }

        setAndroidPermissionDelegate(createAndroidPermissionDelegate());
    }

    protected ActivityAndroidPermissionDelegate createAndroidPermissionDelegate() {
        return new ActivityAndroidPermissionDelegate(getActivity());
    }

    @Override
    protected void registerKeyboardVisibilityCallbacks() {
        Activity activity = getActivity().get();
        if (activity == null) return;
        View content = activity.findViewById(android.R.id.content);
        mIsKeyboardShowing = UiUtils.isKeyboardShowing(getActivity().get(), content);
        content.addOnLayoutChangeListener(this);
    }

    @Override
    protected void unregisterKeyboardVisibilityCallbacks() {
        Activity activity = getActivity().get();
        if (activity == null) return;
        activity.findViewById(android.R.id.content).removeOnLayoutChangeListener(this);
    }

    @Override
    public int showCancelableIntent(
            PendingIntent intent, IntentCallback callback, Integer errorId) {
        Activity activity = getActivity().get();
        if (activity == null) return START_INTENT_FAILURE;

        int requestCode = generateNextRequestCode();

        try {
            activity.startIntentSenderForResult(
                    intent.getIntentSender(), requestCode, new Intent(), 0, 0, 0);
        } catch (SendIntentException e) {
            return START_INTENT_FAILURE;
        }

        storeCallbackData(requestCode, callback, errorId);
        return requestCode;
    }

    @Override
    public int showCancelableIntent(Intent intent, IntentCallback callback, Integer errorId) {
        Activity activity = getActivity().get();
        if (activity == null) return START_INTENT_FAILURE;

        int requestCode = generateNextRequestCode();

        try {
            activity.startActivityForResult(intent, requestCode);
        } catch (ActivityNotFoundException e) {
            return START_INTENT_FAILURE;
        }

        storeCallbackData(requestCode, callback, errorId);
        return requestCode;
    }

    @Override
    public int showCancelableIntent(Callback<Integer> intentTrigger, IntentCallback callback,
            Integer errorId) {
        Activity activity = getActivity().get();
        if (activity == null) return START_INTENT_FAILURE;

        int requestCode = generateNextRequestCode();

        intentTrigger.onResult(requestCode);

        storeCallbackData(requestCode, callback, errorId);
        return requestCode;
    }

    @Override
    public void cancelIntent(int requestCode) {
        Activity activity = getActivity().get();
        if (activity == null) return;
        activity.finishActivity(requestCode);
    }

    /**
     * Responds to the intent result if the intent was created by the native window.
     * @param requestCode Request code of the requested intent.
     * @param resultCode Result code of the requested intent.
     * @param data The data returned by the intent.
     * @return Boolean value of whether the intent was started by the native window.
     */
    public boolean onActivityResult(int requestCode, int resultCode, Intent data) {
        IntentCallback callback = mOutstandingIntents.get(requestCode);
        mOutstandingIntents.delete(requestCode);
        String errorMessage = mIntentErrors.remove(requestCode);

        if (callback != null) {
            callback.onIntentCompleted(this, resultCode, data);
            return true;
        } else {
            if (errorMessage != null) {
                showCallbackNonExistentError(errorMessage);
                return true;
            }
        }
        return false;
    }

    /**
     * Responds to a pending permission result.
     * @param requestCode The unique code for the permission request.
     * @param permissions The list of permissions in the result.
     * @param grantResults Whether the permissions were granted.
     * @return Whether the permission request corresponding to a pending permission request.
     */
    public boolean onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
        getAndroidPermissionDelegate().onRequestPermissionsResult(
                requestCode, permissions, grantResults);
        return true;
    }

    @Override
    public WeakReference<Activity> getActivity() {
        return new WeakReference<Activity>(activityFromContext(getContext().get()));
    }

    @Override
    public void onActivityStateChange(Activity activity, int newState) {
        if (newState == ActivityState.STOPPED) {
            onActivityStopped();
        } else if (newState == ActivityState.STARTED) {
            onActivityStarted();
        }
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
            int oldTop, int oldRight, int oldBottom) {
        keyboardVisibilityPossiblyChanged(UiUtils.isKeyboardShowing(getActivity().get(), v));
    }

    private int generateNextRequestCode() {
        int requestCode = REQUEST_CODE_PREFIX + mNextRequestCode;
        mNextRequestCode = (mNextRequestCode + 1) % REQUEST_CODE_RANGE_SIZE;
        return requestCode;
    }

    private void storeCallbackData(int requestCode, IntentCallback callback, Integer errorId) {
        mOutstandingIntents.put(requestCode, callback);
        mIntentErrors.put(
                requestCode, errorId == null ? null : mApplicationContext.getString(errorId));
    }
}
