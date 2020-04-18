// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import android.text.TextUtils;

import org.junit.Assert;

import org.chromium.base.Callback;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.components.feature_engagement.Tracker;

/**
 * A feature engagement {@link Tracker} for testing.
 */
class FakeTracker implements Tracker {
    public CallbackHelper mDimissedCallbackHelper = new CallbackHelper();

    private String mEnabledFeature;

    public FakeTracker(String enabledFeature) {
        mEnabledFeature = enabledFeature;
    }

    @Override
    public void notifyEvent(String event) {}

    @Override
    public boolean shouldTriggerHelpUI(String feature) {
        return TextUtils.equals(mEnabledFeature, feature);
    }

    @Override
    public int getTriggerState(String feature) {
        return 0;
    }

    @Override
    public void dismissed(String feature) {
        Assert.assertEquals("Wrong feature dismissed.", mEnabledFeature, feature);
        mDimissedCallbackHelper.notifyCalled();
    }

    @Override
    public boolean isInitialized() {
        return true;
    }

    @Override
    public void addOnInitializedCallback(Callback<Boolean> callback) {}

    @Override
    public boolean wouldTriggerHelpUI(String feature) {
        return shouldTriggerHelpUI(feature);
    }

    @Override
    public DisplayLockHandle acquireDisplayLock() {
        return null;
    }
}
