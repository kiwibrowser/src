// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import android.os.SystemClock;
import android.webkit.URLUtil;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tabmodel.TabModel.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;
import org.chromium.chrome.browser.util.UrlUtilities;

import java.util.concurrent.TimeUnit;

/** Class allowing to measure and report a page view time in UMA. */
public class PageViewTimer {
    private final TabModelSelectorTabModelObserver mTabModelObserver;
    private final TabObserver mTabObserver;

    /** Currnetly observed tab. */
    private Tab mCurrentTab;
    /** Last URL loaded in the observed tab. */
    private String mLastUrl;
    /** Start time for the page that is observed. */
    private long mStartTimeMs;
    /** Whether the page is showing anything. */
    private boolean mPageDidPaint;

    public PageViewTimer(TabModelSelector tabModelSelector) {
        // TODO(fgorski): May need to change to TabObserver.
        mTabObserver = new EmptyTabObserver() {
            @Override
            public void onUpdateUrl(Tab tab, String url) {
                assert tab == mCurrentTab;

                // In the current implementation, when the user refreshes a page or navigates to a
                // fragment on the page, it is still part of the same page view.
                if (UrlUtilities.urlsMatchIgnoringFragments(url, mLastUrl)) return;

                maybeReportViewTime();
                maybeStartMeasuring(url, !tab.isLoading());
            }

            @Override
            public void didFirstVisuallyNonEmptyPaint(Tab tab) {
                assert tab == mCurrentTab;
                mPageDidPaint = true;
            }

            @Override
            public void onPageLoadFinished(Tab tab) {
                assert tab == mCurrentTab;
                mPageDidPaint = true;
            }

            @Override
            public void onLoadStopped(Tab tab, boolean toDifferentDocument) {
                assert tab == mCurrentTab;
                mPageDidPaint = true;
            }
        };

        mTabModelObserver = new TabModelSelectorTabModelObserver(tabModelSelector) {
            @Override
            public void didSelectTab(Tab tab, TabSelectionType type, int lastId) {
                assert tab != null;
                if (tab == mCurrentTab) return;

                maybeReportViewTime();
                switchObserverToTab(tab);
                maybeStartMeasuring(tab.getUrl(), !tab.isLoading());
            }

            @Override
            public void willCloseTab(Tab tab, boolean animate) {
                assert tab != null;
                if (tab != mCurrentTab) return;

                maybeReportViewTime();
                switchObserverToTab(null);
            }

            @Override
            public void tabRemoved(Tab tab) {
                assert tab != null;
                if (tab != mCurrentTab) return;

                maybeReportViewTime();
                switchObserverToTab(null);
            }
        };
    }

    /** Destroys the PageViewTimer. */
    public void destroy() {
        maybeReportViewTime();
        switchObserverToTab(null);
        mTabModelObserver.destroy();
    }

    private void maybeReportViewTime() {
        if (mLastUrl != null && mStartTimeMs != 0 && mPageDidPaint) {
            long durationMs = SystemClock.uptimeMillis() - mStartTimeMs;
            RecordHistogram.recordLongTimesHistogram100(
                    "ContextualSuggestions.PageViewTime", durationMs, TimeUnit.MILLISECONDS);
        }

        // Reporting triggers every time the user would see something new, therefore we clean up
        // reporting state every time.
        mLastUrl = null;
        mStartTimeMs = 0;
        mPageDidPaint = false;
    }

    private void switchObserverToTab(Tab tab) {
        if (mCurrentTab != tab && mCurrentTab != null) {
            mCurrentTab.removeObserver(mTabObserver);
        }

        mCurrentTab = tab;
        if (mCurrentTab != null) {
            mCurrentTab.addObserver(mTabObserver);
        }
    }

    private void maybeStartMeasuring(String url, boolean isLoaded) {
        if (!URLUtil.isHttpUrl(url) && !URLUtil.isHttpsUrl(url)) return;

        mLastUrl = url;
        mStartTimeMs = SystemClock.uptimeMillis();
        mPageDidPaint = isLoaded;
    }
}
