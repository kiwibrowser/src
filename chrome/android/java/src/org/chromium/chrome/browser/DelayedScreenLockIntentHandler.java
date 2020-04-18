// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;

import org.chromium.base.ContextUtils;

/**
 * Intent handler that is specific for the situation when the screen is unlocked from pin, pattern,
 * or password.
 * This class handles exactly one intent unlock + dispatch at a time. It could be reused by calling
 * updateDeferredIntent with a new intent.
 */
public class DelayedScreenLockIntentHandler extends BroadcastReceiver {
    private static final int VALID_DEFERRED_PERIOD_MS = 10000;

    private final Handler mTaskHandler;
    private final Runnable mUnregisterTask;

    private Intent mDeferredIntent;
    private boolean mReceiverRegistered;

    public DelayedScreenLockIntentHandler() {
        mTaskHandler = new Handler();
        mUnregisterTask = () -> updateDeferredIntent(null);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        assert Intent.ACTION_USER_PRESENT.equals(intent.getAction());

        if (Intent.ACTION_USER_PRESENT.equals(intent.getAction()) && mDeferredIntent != null) {
            context.startActivity(mDeferredIntent);
            // Prevent the broadcast receiver from firing intent unexpectedly.
            updateDeferredIntent(null);
        }
    }

    /**
     * Update the deferred intent with the target intent, also reset the deferred intent's lifecycle
     * @param intent Target intent
     */
    public void updateDeferredIntent(final Intent intent) {
        mTaskHandler.removeCallbacks(mUnregisterTask);

        if (intent == null) {
            unregisterReceiver();
            mDeferredIntent = null;
            return;
        }

        mDeferredIntent = intent;
        registerReceiver();
        mTaskHandler.postDelayed(mUnregisterTask, VALID_DEFERRED_PERIOD_MS);
    }

    /**
     * Register to receive ACTION_USER_PRESENT when the screen is unlocked.
     * The ACTION_USER_PRESENT is sent by platform to indicates when user is present.
     */
    private void registerReceiver() {
        if (mReceiverRegistered) return;

        ContextUtils.getApplicationContext()
                .registerReceiver(this, new IntentFilter(Intent.ACTION_USER_PRESENT));
        mReceiverRegistered = true;
    }

    /**
     * Unregister the receiver in one of the following situations
     * - When the deferred intent expires
     * - When updateDeferredIntent(null) called
     * - When the deferred intent has been fired
     */
    private void unregisterReceiver() {
        if (!mReceiverRegistered) return;

        ContextUtils.getApplicationContext().unregisterReceiver(this);
        mReceiverRegistered = false;
    }
}
