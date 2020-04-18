// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.v7.widget.RecyclerView;

import org.chromium.base.Callback;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.FaviconFetchResult;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.tab.Tab;

import java.util.concurrent.TimeUnit;

/**
 * Exposes methods to report suggestions related events, for UMA or Fetch scheduling purposes.
 */
public abstract class SuggestionsMetrics {
    private SuggestionsMetrics() {}

    // UI Element interactions

    public static void recordSurfaceVisible() {
        if (!ChromePreferenceManager.getInstance().getSuggestionsSurfaceShown()) {
            RecordUserAction.record("Suggestions.FirstTimeSurfaceVisible");
            ChromePreferenceManager.getInstance().setSuggestionsSurfaceShown();
        }

        RecordUserAction.record("Suggestions.SurfaceVisible");
    }

    public static void recordSurfaceHalfVisible() {
        RecordUserAction.record("Suggestions.SurfaceHalfVisible");
    }

    public static void recordSurfaceFullyVisible() {
        RecordUserAction.record("Suggestions.SurfaceFullyVisible");
    }

    public static void recordSurfaceHidden() {
        RecordUserAction.record("Suggestions.SurfaceHidden");
    }

    public static void recordTileTapped() {
        RecordUserAction.record("Suggestions.Tile.Tapped");
    }

    public static void recordExpandableHeaderTapped(boolean expanded) {
        if (expanded) {
            RecordUserAction.record("Suggestions.ExpandableHeader.Expanded");
        } else {
            RecordUserAction.record("Suggestions.ExpandableHeader.Collapsed");
        }
    }

    public static void recordCardTapped() {
        RecordUserAction.record("Suggestions.Card.Tapped");
    }

    public static void recordCardActionTapped() {
        RecordUserAction.record("Suggestions.Card.ActionTapped");
    }

    public static void recordCardSwipedAway() {
        RecordUserAction.record("Suggestions.Card.SwipedAway");
    }

    public static void recordContextualSuggestionOpened() {
        RecordUserAction.record("Suggestions.ContextualSuggestion.Open");
    }

    public static void recordContextualSuggestionsCarouselShown() {
        RecordUserAction.record("Suggestions.Contextual.Carousel.Shown");
    }

    public static void recordContextualSuggestionsCarouselScrolled() {
        RecordUserAction.record("Suggestions.Contextual.Carousel.Scrolled");
    }

    // Effect/Purpose of the interactions. Most are recorded in |content_suggestions_metrics.h|

    public static void recordActionViewAll() {
        RecordUserAction.record("Suggestions.Category.ViewAll");
    }

    /**
     * Records metrics for the visit to the provided content suggestion, such as the time spent on
     * the website, or if the user comes back to the starting point.
     * @param tab The tab we want to record the visit on. It should have a live WebContents.
     * @param suggestion The suggestion that prompted the visit.
     */
    public static void recordVisit(Tab tab, SnippetArticle suggestion) {
        @CategoryInt
        final int category = suggestion.mCategory;
        NavigationRecorder.record(tab, visit -> {
            if (NewTabPage.isNTPUrl(visit.endUrl)) {
                RecordUserAction.record("MobileNTP.Snippets.VisitEndBackInNTP");
            }
            RecordUserAction.record("MobileNTP.Snippets.VisitEnd");
            SuggestionsEventReporterBridge.onSuggestionTargetVisited(category, visit.duration);
        });
    }

    // Histogram recordings

    /**
     * Records whether article suggestions are set visible by user.
     */
    public static void recordArticlesListVisible() {
        if (!ChromeFeatureList.isEnabled(
                    ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER)) {
            return;
        }

        RecordHistogram.recordBooleanHistogram("NewTabPage.ContentSuggestions.ArticlesListVisible",
                PrefServiceBridge.getInstance().getBoolean(Pref.NTP_ARTICLES_LIST_VISIBLE));
    }

    /**
     * Records the time it took to fetch a favicon for an article.
     *
     * @param fetchTime The time it took to fetch the favicon.
     */
    public static void recordArticleFaviconFetchTime(long fetchTime) {
        RecordHistogram.recordMediumTimesHistogram(
                "NewTabPage.ContentSuggestions.ArticleFaviconFetchTime", fetchTime,
                TimeUnit.MILLISECONDS);
    }

    /**
     * Records the result from a favicon fetch for an article.
     *
     * @param result {@link FaviconFetchResult} The result from the fetch.
     */
    public static void recordArticleFaviconFetchResult(@FaviconFetchResult int result) {
        RecordHistogram.recordEnumeratedHistogram(
                "NewTabPage.ContentSuggestions.ArticleFaviconFetchResult", result,
                FaviconFetchResult.COUNT);
    }

    /**
     * Records which tiles are available offline once the site suggestions finished loading.
     * @param tileIndex index of a tile whose URL is available offline.
     */
    public static void recordTileOfflineAvailability(int tileIndex) {
        RecordHistogram.recordEnumeratedHistogram("NewTabPage.TileOfflineAvailable", tileIndex,
                MostVisitedSitesBridge.MAX_TILE_COUNT);
    }

    /**
     * @return A {@link DurationTracker} to notify to report how long the spinner is visible
     * for.
     */
    public static DurationTracker getSpinnerVisibilityReporter() {
        return new DurationTracker((duration) -> {
            RecordHistogram.recordTimesHistogram(
                    "ContentSuggestions.FetchPendingSpinner.VisibleDuration", duration,
                    TimeUnit.MILLISECONDS);
        });
    }

    /**
     * Measures the amount of time it takes for date formatting in order to track StrictMode
     * violations.
     * See https://crbug.com/639877
     * @param duration Duration of date formatting.
     */
    static void recordDateFormattingDuration(long duration) {
        RecordHistogram.recordTimesHistogram(
                "Android.StrictMode.SnippetUIBuildTime", duration, TimeUnit.MILLISECONDS);
    }

    /**
     * One-shot reporter that records the first time the user scrolls a {@link RecyclerView}. If it
     * should be reused, call {@link #reset()} to rearm it.
     */
    public static class ScrollEventReporter extends RecyclerView.OnScrollListener {
        private boolean mFired;
        @Override
        public void onScrollStateChanged(RecyclerView recyclerView, int newState) {
            if (mFired) return;
            if (newState != RecyclerView.SCROLL_STATE_DRAGGING) return;

            RecordUserAction.record("Suggestions.ScrolledAfterOpen");
            mFired = true;
        }

        public void reset() {
            mFired = false;
        }
    }

    /**
     * Utility class to track the duration of an event. Call {@link #startTracking()} and
     * {@link #endTracking()} to notify about the key moments. These methods are no-ops when called
     * while tracking is not in the expected state.
     */
    public static class DurationTracker {
        private long mTrackingStartTimeMs;
        private final Callback<Long> mTrackingCompleteCallback;

        private DurationTracker(Callback<Long> trackingCompleteCallback) {
            mTrackingCompleteCallback = trackingCompleteCallback;
        }

        public void startTracking() {
            if (isTracking()) return;
            mTrackingStartTimeMs = System.currentTimeMillis();
        }

        public void endTracking() {
            if (!isTracking()) return;
            mTrackingCompleteCallback.onResult(System.currentTimeMillis() - mTrackingStartTimeMs);
            mTrackingStartTimeMs = 0;
        }

        private boolean isTracking() {
            return mTrackingStartTimeMs > 0;
        }
    }
}
