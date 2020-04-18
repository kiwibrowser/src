// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.os.Bundle;

import org.chromium.base.test.util.CallbackHelper;

/** Records when the FirstRunActivity has progressed through various states. */
public class FirstRunActivityTestObserver implements FirstRunActivity.FirstRunActivityObserver {
    public final CallbackHelper flowIsKnownCallback = new CallbackHelper();
    public final CallbackHelper acceptTermsOfServiceCallback = new CallbackHelper();
    public final CallbackHelper jumpToPageCallback = new CallbackHelper();
    public final CallbackHelper updateCachedEngineCallback = new CallbackHelper();
    public final CallbackHelper abortFirstRunExperienceCallback = new CallbackHelper();

    public Bundle freProperties;

    @Override
    public void onFlowIsKnown(Bundle freProperties) {
        this.freProperties = freProperties;
        flowIsKnownCallback.notifyCalled();
    }

    @Override
    public void onAcceptTermsOfService() {
        acceptTermsOfServiceCallback.notifyCalled();
    }

    @Override
    public void onJumpToPage(int position) {
        jumpToPageCallback.notifyCalled();
    }

    @Override
    public void onUpdateCachedEngineName() {
        updateCachedEngineCallback.notifyCalled();
    }

    @Override
    public void onAbortFirstRunExperience() {
        abortFirstRunExperienceCallback.notifyCalled();
    }
}