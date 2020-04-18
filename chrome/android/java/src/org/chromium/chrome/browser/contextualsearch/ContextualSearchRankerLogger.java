// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.support.annotation.Nullable;

import org.chromium.content_public.browser.WebContents;

/**
 * An interface for logging to UMA via Ranker.
 */
public interface ContextualSearchRankerLogger {
    // TODO(donnd): consider changing this enum to an IntDef.
    // NOTE: this list needs to be kept in sync with the white list in
    // predictor_config_definitions.cc, the names list in ContextualSearchRankerLoggerImpl.java
    // and with ukm.xml!
    enum Feature {
        UNKNOWN,
        // Outcome labels:
        OUTCOME_WAS_PANEL_OPENED,
        OUTCOME_WAS_QUICK_ACTION_CLICKED,
        OUTCOME_WAS_QUICK_ANSWER_SEEN,
        OUTCOME_WAS_CARDS_DATA_SHOWN, // a UKM CS v2 label.
        // Features:
        DURATION_AFTER_SCROLL_MS,
        SCREEN_TOP_DPS,
        WAS_SCREEN_BOTTOM,
        // User usage features:
        PREVIOUS_WEEK_IMPRESSIONS_COUNT,
        PREVIOUS_WEEK_CTR_PERCENT,
        PREVIOUS_28DAY_IMPRESSIONS_COUNT,
        PREVIOUS_28DAY_CTR_PERCENT,
        // UKM CS v2 features (see go/ukm-cs-2).
        DID_OPT_IN,
        IS_SHORT_WORD,
        IS_LONG_WORD,
        IS_WORD_EDGE,
        IS_ENTITY,
        TAP_DURATION_MS,
        // UKM CS v3 features (see go/ukm-cs-3).
        FONT_SIZE,
        IS_SECOND_TAP_OVERRIDE,
        IS_HTTP,
        IS_ENTITY_ELIGIBLE,
        IS_LANGUAGE_MISMATCH,
        PORTION_OF_ELEMENT
    }

    /**
     * Sets up logging for the base page which is identified by the given {@link WebContents}.
     * This method must be called before calling {@link #logFeature} or {@link #logOutcome}.
     * @param basePageWebContents The {@link WebContents} of the base page to log with Ranker.
     */
    void setupLoggingForPage(@Nullable WebContents basePageWebContents);

    /**
     * Logs a particular feature at inference time as a key/value pair.
     * @param feature The feature to log.
     * @param value The value to log, which is associated with the given key.
     */
    void logFeature(Feature feature, Object value);

    /**
     * Returns whether or not AssistRanker query is enabled.
     */
    boolean isQueryEnabled();

    /**
     * Logs an outcome value at training time that indicates an ML label as a key/value pair.
     * @param feature The feature to log.
     * @param value The outcome label value.
     */
    void logOutcome(Feature feature, Object value);

    /**
     * Tries to run the machine intelligence model for tap suppression and returns an int that
     * describes whether the prediction was obtainable and what it was.
     * See chrome/browser/android/contextualsearch/contextual_search_ranker_logger_impl.h for
     * details on the {@link AssistRankerPrediction} possibilities.
     * @return An integer that encodes the prediction result.
     */
    @AssistRankerPrediction
    int runPredictionForTapSuppression();

    /**
     * Gets the previous result from trying to run the machine intelligence model for tap
     * suppression. A previous call to {@link #runPredictionForTapSuppression} is required.
     * See chrome/browser/android/contextualsearch/contextual_search_ranker_logger_impl.h for
     * details on the {@link AssistRankerPrediction} possibilities.
     * @return An integer that encodes the prediction.
     */
    @AssistRankerPrediction
    int getPredictionForTapSuppression();

    /**
     * Resets the logger so that future log calls accumulate into a new record.
     * Any accumulated logging for the current record is discarded.
     */
    void reset();

    /**
     * Writes all the accumulated log entries and resets the logger so that future log calls
     * accumulate into a new record. This can be called multiple times without side-effects when
     * nothing new has been written to the log.
     * After calling this method another call to {@link #setupLoggingForPage} is required before
     * additional {@link #logFeature} or {@link #logOutcome} calls.
     */
    void writeLogAndReset();
}
