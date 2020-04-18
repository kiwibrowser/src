// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import android.app.Application;
import android.graphics.Rect;
import android.net.Uri;
import android.os.SystemClock;
import android.support.customtabs.CustomTabsSessionToken;
import android.text.TextUtils;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.prerender.ExternalPrerenderHandler;
import org.chromium.chrome.browser.share.ShareHelper;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.components.security_state.ConnectionSecurityLevel;
import org.chromium.content_public.browser.LoadUrlParams;

import java.util.concurrent.TimeUnit;

/**
 * A {@link TabObserver} that also handles custom tabs specific logging and messaging.
 */
class CustomTabObserver extends EmptyTabObserver {
    private final CustomTabsConnection mCustomTabsConnection;
    private final CustomTabsSessionToken mSession;
    private final boolean mOpenedByChrome;
    private final NavigationInfoCaptureTrigger mNavigationInfoCaptureTrigger =
            new NavigationInfoCaptureTrigger(this::captureNavigationInfo);
    private int mContentBitmapWidth;
    private int mContentBitmapHeight;

    private long mIntentReceivedTimestamp;
    private long mPageLoadStartedTimestamp;
    private long mFirstCommitTimestamp;

    private static final int STATE_RESET = 0;
    private static final int STATE_WAITING_LOAD_START = 1;
    private static final int STATE_WAITING_LOAD_FINISH = 2;
    private int mCurrentState;

    public CustomTabObserver(
            Application application, CustomTabsSessionToken session, boolean openedByChrome) {
        if (openedByChrome) {
            mCustomTabsConnection = null;
        } else {
            mCustomTabsConnection = CustomTabsConnection.getInstance();
        }
        mSession = session;
        if (!openedByChrome && mCustomTabsConnection.shouldSendNavigationInfoForSession(mSession)) {
            float desiredWidth = application.getResources().getDimensionPixelSize(
                    R.dimen.custom_tabs_screenshot_width);
            float desiredHeight = application.getResources().getDimensionPixelSize(
                    R.dimen.custom_tabs_screenshot_height);
            Rect bounds = ExternalPrerenderHandler.estimateContentSize(application, false);
            if (bounds.width() == 0 || bounds.height() == 0) {
                mContentBitmapWidth = Math.round(desiredWidth);
                mContentBitmapHeight = Math.round(desiredHeight);
            } else {
                // Compute a size that scales the content bitmap to fit one (or both) dimensions,
                // but also preserves aspect ratio.
                float scale =
                        Math.min(desiredWidth / bounds.width(), desiredHeight / bounds.height());
                mContentBitmapWidth = Math.round(bounds.width() * scale);
                mContentBitmapHeight = Math.round(bounds.height() * scale);
            }
        }
        mOpenedByChrome = openedByChrome;
        resetPageLoadTracking();
    }

    /**
     * Tracks the next page load, with timestamp as the origin of time.
     * If a load is already happening, we track its PLT.
     * If not, we track NavigationCommit timing + PLT for the next load.
     */
    public void trackNextPageLoadFromTimestamp(Tab tab, long timestamp) {
        mIntentReceivedTimestamp = timestamp;
        if (tab.isLoading()) {
            mPageLoadStartedTimestamp = -1;
            mCurrentState = STATE_WAITING_LOAD_FINISH;
        } else {
            mCurrentState = STATE_WAITING_LOAD_START;
        }
    }

    @Override
    public void onLoadUrl(Tab tab, LoadUrlParams params, int loadType) {
        if (mCustomTabsConnection != null) {
            mCustomTabsConnection.registerLaunch(mSession, params.getUrl());
        }
    }

    @Override
    public void onPageLoadStarted(Tab tab, String url) {
        if (mCurrentState == STATE_WAITING_LOAD_START) {
            mPageLoadStartedTimestamp = SystemClock.elapsedRealtime();
            mCurrentState = STATE_WAITING_LOAD_FINISH;
        } else if (mCurrentState == STATE_WAITING_LOAD_FINISH) {
            if (mCustomTabsConnection != null) {
                mCustomTabsConnection.sendNavigationInfo(
                        mSession, tab.getUrl(), tab.getTitle(), (Uri) null);
            }
            mPageLoadStartedTimestamp = SystemClock.elapsedRealtime();
        }
        if (mCustomTabsConnection != null) {
            mCustomTabsConnection.setSendNavigationInfoForSession(mSession, false);
            mNavigationInfoCaptureTrigger.onNewNavigation();
        }
    }

    @Override
    public void onHidden(Tab tab) {
        mNavigationInfoCaptureTrigger.onHide(tab);
    }

    @Override
    public void onPageLoadFinished(Tab tab) {
        long pageLoadFinishedTimestamp = SystemClock.elapsedRealtime();

        if (mCurrentState == STATE_WAITING_LOAD_FINISH && mIntentReceivedTimestamp > 0) {
            String histogramPrefix = mOpenedByChrome ? "ChromeGeneratedCustomTab" : "CustomTabs";
            long timeToPageLoadFinishedMs = pageLoadFinishedTimestamp - mIntentReceivedTimestamp;
            if (mPageLoadStartedTimestamp > 0) {
                long timeToPageLoadStartedMs = mPageLoadStartedTimestamp - mIntentReceivedTimestamp;
                // Intent to Load Start is recorded here to make sure we do not record
                // failed/aborted page loads.
                RecordHistogram.recordCustomTimesHistogram(
                        histogramPrefix + ".IntentToFirstNavigationStartTime.ZoomedOut",
                        timeToPageLoadStartedMs, 50, TimeUnit.MINUTES.toMillis(10),
                        TimeUnit.MILLISECONDS, 50);
                RecordHistogram.recordCustomTimesHistogram(
                        histogramPrefix + ".IntentToFirstNavigationStartTime.ZoomedIn",
                        timeToPageLoadStartedMs, 200, 1000, TimeUnit.MILLISECONDS, 100);
            }
            // Same bounds and bucket count as PLT histograms.
            RecordHistogram.recordCustomTimesHistogram(histogramPrefix + ".IntentToPageLoadedTime",
                    timeToPageLoadFinishedMs, 10, TimeUnit.MINUTES.toMillis(10),
                    TimeUnit.MILLISECONDS, 100);

            // Not all page loads go through a navigation commit (prerender for instance).
            if (mPageLoadStartedTimestamp != 0) {
                long timeToFirstCommitMs = mFirstCommitTimestamp - mIntentReceivedTimestamp;
                // Current median is 550ms, and long tail is very long. ZoomedIn gives good view of
                // the median and ZoomedOut gives a good overview.
                RecordHistogram.recordCustomTimesHistogram(
                        "CustomTabs.IntentToFirstCommitNavigationTime3.ZoomedIn",
                        timeToFirstCommitMs, 200, 1000, TimeUnit.MILLISECONDS, 100);
                // For ZoomedOut very rarely is it under 50ms and this range matches
                // CustomTabs.IntentToFirstCommitNavigationTime2.ZoomedOut.
                RecordHistogram.recordCustomTimesHistogram(
                        "CustomTabs.IntentToFirstCommitNavigationTime3.ZoomedOut",
                        timeToFirstCommitMs, 50, TimeUnit.MINUTES.toMillis(10),
                        TimeUnit.MILLISECONDS, 50);
            }
        }
        resetPageLoadTracking();
        mNavigationInfoCaptureTrigger.onLoadFinished(tab);
    }

    @Override
    public void onDidAttachInterstitialPage(Tab tab) {
        if (tab.getSecurityLevel() != ConnectionSecurityLevel.DANGEROUS) return;
        resetPageLoadTracking();
    }

    @Override
    public void onPageLoadFailed(Tab tab, int errorCode) {
        resetPageLoadTracking();
    }

    @Override
    public void onDidFinishNavigation(Tab tab, String url, boolean isInMainFrame,
            boolean isErrorPage, boolean hasCommitted, boolean isSameDocument,
            boolean isFragmentNavigation, Integer pageTransition, int errorCode,
            int httpStatusCode) {
        boolean firstNavigation = mFirstCommitTimestamp == 0;
        boolean isFirstMainFrameCommit = firstNavigation && hasCommitted && !isErrorPage
                && isInMainFrame && !isSameDocument && !isFragmentNavigation;
        if (isFirstMainFrameCommit) mFirstCommitTimestamp = SystemClock.elapsedRealtime();
    }

    public void onFirstMeaningfulPaint(Tab tab) {
        mNavigationInfoCaptureTrigger.onFirstMeaningfulPaint(tab);
    }

    private void resetPageLoadTracking() {
        mCurrentState = STATE_RESET;
        mIntentReceivedTimestamp = -1;
    }

    private void captureNavigationInfo(final Tab tab) {
        if (mCustomTabsConnection == null) return;
        if (!mCustomTabsConnection.shouldSendNavigationInfoForSession(mSession)) return;
        if (tab.getWebContents() == null) return;

        ShareHelper.captureScreenshotForContents(tab.getWebContents(), mContentBitmapWidth,
                mContentBitmapHeight, (Uri snapshotPath) -> {
                    if (TextUtils.isEmpty(tab.getTitle()) && snapshotPath == null) return;
                    mCustomTabsConnection.sendNavigationInfo(
                            mSession, tab.getUrl(), tab.getTitle(), snapshotPath);
                });
    }
}
