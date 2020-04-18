// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import org.chromium.content.browser.test.util.TestCallbackHelperContainer.OnPageFinishedHelper;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer.OnPageStartedHelper;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer.OnReceivedErrorHelper;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;

/**
 * The default WebContentsObserver used by ContentView tests. The below callbacks can be
 * accessed by using {@link TestCallbackHelperContainer} or extending this class.
 */
public class TestWebContentsObserver extends WebContentsObserver {

    private final OnPageStartedHelper mOnPageStartedHelper;
    private final OnPageFinishedHelper mOnPageFinishedHelper;
    private final OnReceivedErrorHelper mOnReceivedErrorHelper;

    public TestWebContentsObserver(WebContents webContents) {
        super(webContents);
        mOnPageStartedHelper = new OnPageStartedHelper();
        mOnPageFinishedHelper = new OnPageFinishedHelper();
        mOnReceivedErrorHelper = new OnReceivedErrorHelper();
    }

    public OnPageStartedHelper getOnPageStartedHelper() {
        return mOnPageStartedHelper;
    }

    public OnPageFinishedHelper getOnPageFinishedHelper() {
        return mOnPageFinishedHelper;
    }

    public OnReceivedErrorHelper getOnReceivedErrorHelper() {
        return mOnReceivedErrorHelper;
    }

    /**
     * ATTENTION!: When overriding the following methods, be sure to call
     * the corresponding methods in the super class. Otherwise
     * {@link CallbackHelper#waitForCallback()} methods will
     * stop working!
     */
    @Override
    public void didStartLoading(String url) {
        super.didStartLoading(url);
        mOnPageStartedHelper.notifyCalled(url);
    }

    @Override
    public void didStopLoading(String url) {
        super.didStopLoading(url);
        mOnPageFinishedHelper.notifyCalled(url);
    }

    @Override
    public void didFailLoad(
            boolean isMainFrame, int errorCode, String description, String failingUrl) {
        super.didFailLoad(isMainFrame, errorCode, description, failingUrl);
        mOnReceivedErrorHelper.notifyCalled(errorCode, description, failingUrl);
    }
}
