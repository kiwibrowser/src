// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.dom_distiller;

import android.os.SystemClock;

import org.chromium.content_public.browser.WebContentsObserver;

/**
 * This class tracks the per-tab state of reader mode.
 */
public class ReaderModeTabInfo {
    // The WebContentsObserver responsible for updates to the distillation status of the tab.
    private WebContentsObserver mWebContentsObserver;

    // The distillation status of the tab.
    private int mStatus;

    // If the infobar was closed due to the close button.
    private boolean mIsDismissed;

    // The URL that distiller is using for this tab. This is used to check if a result comes
    // back from distiller and the user has already loaded a new URL.
    private String mCurrentUrl;

    // The distillability heuristics now use a callback to notify the manager that a page can
    // be distilled. This flag is used to detect if the callback is set for this tab.
    private boolean mIsCallbackSet;

    // Used to flag the the infobar was shown and recorded by UMA.
    private boolean mShowInfoBarRecorded;

    // The time that the user started viewing Reader Mode content.
    private long mViewStartTimeMs;

    // Whether or not the current tab is a Reader Mode page.
    private boolean mIsViewingReaderModePage;

    /**
     * @param observer The WebContentsObserver for the tab this object represents.
     */
    public void setWebContentsObserver(WebContentsObserver observer) {
        mWebContentsObserver = observer;
    }

    /**
     * @return The WebContentsObserver for the tab this object represents.
     */
    public WebContentsObserver getWebContentsObserver() {
        return mWebContentsObserver;
    }

    /**
     * A notification that the user started viewing Reader Mode.
     */
    public void onStartedReaderMode() {
        mIsViewingReaderModePage = true;
        mViewStartTimeMs = SystemClock.elapsedRealtime();
    }

    /**
     * A notification that the user is no longer viewing Reader Mode. This could be because of a
     * navigation away from the page, switching tabs, or closing the browser.
     * @return The amount of time in ms that the user spent viewing Reader Mode.
     */
    public long onExitReaderMode() {
        mIsViewingReaderModePage = false;
        return SystemClock.elapsedRealtime() - mViewStartTimeMs;
    }

    /**
     * @return Whether or not the user is on a Reader Mode page.
     */
    public boolean isViewingReaderModePage() {
        return mIsViewingReaderModePage;
    }

    /**
     * @param status The status of reader mode for this object's tab.
     */
    public void setStatus(int status) {
        mStatus = status;
    }

    /**
     * @return The reader mode status for this object's tab.
     */
    public int getStatus() {
        return mStatus;
    }

    /**
     * @return If the infobar has been dismissed for this object's tab.
     */
    public boolean isDismissed() {
        return mIsDismissed;
    }

    /**
     * @param dismissed Set the infobar as dismissed for this object's tab.
     */
    public void setIsDismissed(boolean dismissed) {
        mIsDismissed = dismissed;
    }

    /**
     * @param url The URL being processed by reader mode.
     */
    public void setUrl(String url) {
        mCurrentUrl = url;
    }

    /**
     * @return The last URL being processed by reader mode.
     */
    public String getUrl() {
        return mCurrentUrl;
    }

    /**
     * @return If the distillability callback is set for this object's tab.
     */
    public boolean isCallbackSet() {
        return mIsCallbackSet;
    }

    /**
     * @param isSet Set if this object's tab has a distillability callback.
     */
    public void setIsCallbackSet(boolean isSet) {
        mIsCallbackSet = isSet;
    }

    /**
     * @return If the call to show the infobar was recorded.
     */
    public boolean isInfoBarShowRecorded() {
        return mShowInfoBarRecorded;
    }

    /**
     * @param isRecorded True if the action has been recorded.
     */
    public void setIsInfoBarShowRecorded(boolean isRecorded) {
        mShowInfoBarRecorded = isRecorded;
    }

}

