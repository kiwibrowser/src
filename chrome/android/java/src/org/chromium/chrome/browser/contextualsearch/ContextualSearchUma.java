// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.util.Pair;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.browser.compositor.bottombar.OverlayPanel.PanelState;
import org.chromium.chrome.browser.compositor.bottombar.OverlayPanel.StateChangeReason;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Centralizes UMA data collection for Contextual Search. All calls must be made from the UI thread.
 */
public class ContextualSearchUma {
    // Constants to use for the original selection gesture
    private static final boolean LONG_PRESS = false;
    private static final boolean TAP = true;

    // Constants used to log UMA "enum" histograms about the Contextual Search's preference state.
    private static final int PREFERENCE_UNINITIALIZED = 0;
    private static final int PREFERENCE_ENABLED = 1;
    private static final int PREFERENCE_DISABLED = 2;
    private static final int PREFERENCE_HISTOGRAM_BOUNDARY = 3;

    // Constants used to log UMA "enum" histograms about whether search results were seen.
    private static final int RESULTS_SEEN = 0;
    private static final int RESULTS_NOT_SEEN = 1;
    private static final int RESULTS_SEEN_BOUNDARY = 2;

    // Constants used to log UMA "enum" histograms about whether the selection is valid.
    private static final int SELECTION_VALID = 0;
    private static final int SELECTION_INVALID = 1;
    private static final int SELECTION_BOUNDARY = 2;

    // Constants used to log UMA "enum" histograms about a request's outcome.
    private static final int REQUEST_NOT_FAILED = 0;
    private static final int REQUEST_FAILED = 1;
    private static final int REQUEST_BOUNDARY = 2;

    // Constants used to log UMA "enum" histograms about the panel's state transitions.
    // Entry code: first entry into CLOSED.
    private static final int ENTER_CLOSED_FROM_OTHER = 0;
    private static final int ENTER_CLOSED_FROM_PEEKED_BACK_PRESS = 1;
    private static final int ENTER_CLOSED_FROM_PEEKED_BASE_PAGE_SCROLL = 2;
    private static final int ENTER_CLOSED_FROM_PEEKED_TEXT_SELECT_TAP = 3;
    private static final int ENTER_CLOSED_FROM_EXPANDED_BACK_PRESS = 4;
    private static final int ENTER_CLOSED_FROM_EXPANDED_BASE_PAGE_TAP = 5;
    private static final int ENTER_CLOSED_FROM_EXPANDED_FLING = 6;
    private static final int ENTER_CLOSED_FROM_MAXIMIZED_BACK_PRESS = 7;
    private static final int ENTER_CLOSED_FROM_MAXIMIZED_FLING = 8;
    private static final int ENTER_CLOSED_FROM_MAXIMIZED_TAB_PROMOTION = 9;
    private static final int ENTER_CLOSED_FROM_MAXIMIZED_SERP_NAVIGATION = 10;
    private static final int ENTER_CLOSED_FROM_BOUNDARY = 11;

    // Entry code: first entry into PEEKED.
    private static final int ENTER_PEEKED_FROM_OTHER = 0;
    private static final int ENTER_PEEKED_FROM_CLOSED_TEXT_SELECT_TAP = 1;
    private static final int ENTER_PEEKED_FROM_CLOSED_EXT_SELECT_LONG_PRESS = 2;
    private static final int ENTER_PEEKED_FROM_PEEKED_TEXT_SELECT_TAP = 3;
    private static final int ENTER_PEEKED_FROM_PEEKED_TEXT_SELECT_LONG_PRESS = 4;
    private static final int ENTER_PEEKED_FROM_EXPANDED_SEARCH_BAR_TAP = 5;
    private static final int ENTER_PEEKED_FROM_EXPANDED_SWIPE = 6;
    private static final int ENTER_PEEKED_FROM_EXPANDED_FLING = 7;
    private static final int ENTER_PEEKED_FROM_MAXIMIZED_SWIPE = 8;
    private static final int ENTER_PEEKED_FROM_MAXIMIZED_FLING = 9;
    private static final int ENTER_PEEKED_FROM_BOUNDARY = 10;

    // Entry code: first entry into EXPANDED.
    private static final int ENTER_EXPANDED_FROM_OTHER = 0;
    private static final int ENTER_EXPANDED_FROM_PEEKED_SEARCH_BAR_TAP = 1;
    private static final int ENTER_EXPANDED_FROM_PEEKED_SWIPE = 2;
    private static final int ENTER_EXPANDED_FROM_PEEKED_FLING = 3;
    private static final int ENTER_EXPANDED_FROM_MAXIMIZED_SWIPE = 4;
    private static final int ENTER_EXPANDED_FROM_MAXIMIZED_FLING = 5;
    private static final int ENTER_EXPANDED_FROM_BOUNDARY = 6;

    // Entry code: first entry into MAXIMIZED.
    private static final int ENTER_MAXIMIZED_FROM_OTHER = 0;
    private static final int ENTER_MAXIMIZED_FROM_PEEKED_SWIPE = 1;
    private static final int ENTER_MAXIMIZED_FROM_PEEKED_FLING = 2;
    private static final int ENTER_MAXIMIZED_FROM_EXPANDED_SWIPE = 3;
    private static final int ENTER_MAXIMIZED_FROM_EXPANDED_FLING = 4;
    private static final int ENTER_MAXIMIZED_FROM_EXPANDED_SERP_NAVIGATION = 5;
    private static final int ENTER_MAXIMIZED_FROM_BOUNDARY = 6;

    // Exit code: first exit from CLOSED (or UNDEFINED).
    private static final int EXIT_CLOSED_TO_OTHER = 0;
    private static final int EXIT_CLOSED_TO_PEEKED_TEXT_SELECT_TAP = 1;
    private static final int EXIT_CLOSED_TO_PEEKED_TEXT_SELECT_LONG_PRESS = 2;
    private static final int EXIT_CLOSED_TO_BOUNDARY = 3;

    // Exit code: first exit from PEEKED.
    private static final int EXIT_PEEKED_TO_OTHER = 0;
    private static final int EXIT_PEEKED_TO_CLOSED_BACK_PRESS = 1;
    private static final int EXIT_PEEKED_TO_CLOSED_BASE_PAGE_SCROLL = 2;
    private static final int EXIT_PEEKED_TO_CLOSED_TEXT_SELECT_TAP = 3;
    private static final int EXIT_PEEKED_TO_PEEKED_TEXT_SELECT_TAP = 4;
    private static final int EXIT_PEEKED_TO_PEEKED_TEXT_SELECT_LONG_PRESS = 5;
    private static final int EXIT_PEEKED_TO_EXPANDED_SEARCH_BAR_TAP = 6;
    private static final int EXIT_PEEKED_TO_EXPANDED_SWIPE = 7;
    private static final int EXIT_PEEKED_TO_EXPANDED_FLING = 8;
    private static final int EXIT_PEEKED_TO_MAXIMIZED_SWIPE = 9;
    private static final int EXIT_PEEKED_TO_MAXIMIZED_FLING = 10;
    private static final int EXIT_PEEKED_TO_BOUNDARY = 11;

    // Exit code: first exit from EXPANDED.
    private static final int EXIT_EXPANDED_TO_OTHER = 0;
    private static final int EXIT_EXPANDED_TO_CLOSED_BACK_PRESS = 1;
    private static final int EXIT_EXPANDED_TO_CLOSED_BASE_PAGE_TAP = 2;
    private static final int EXIT_EXPANDED_TO_CLOSED_FLING = 3;
    private static final int EXIT_EXPANDED_TO_PEEKED_SEARCH_BAR_TAP = 4;
    private static final int EXIT_EXPANDED_TO_PEEKED_SWIPE = 5;
    private static final int EXIT_EXPANDED_TO_PEEKED_FLING = 6;
    private static final int EXIT_EXPANDED_TO_MAXIMIZED_SWIPE = 7;
    private static final int EXIT_EXPANDED_TO_MAXIMIZED_FLING = 8;
    private static final int EXIT_EXPANDED_TO_MAXIMIZED_SERP_NAVIGATION = 9;
    private static final int EXIT_EXPANDED_TO_BOUNDARY = 10;

    // Exit code: first exit from MAXIMIZED.
    private static final int EXIT_MAXIMIZED_TO_OTHER = 0;
    private static final int EXIT_MAXIMIZED_TO_CLOSED_BACK_PRESS = 1;
    private static final int EXIT_MAXIMIZED_TO_CLOSED_FLING = 2;
    private static final int EXIT_MAXIMIZED_TO_CLOSED_TAB_PROMOTION = 3;
    private static final int EXIT_MAXIMIZED_TO_CLOSED_SERP_NAVIGATION = 4;
    private static final int EXIT_MAXIMIZED_TO_PEEKED_SWIPE = 5;
    private static final int EXIT_MAXIMIZED_TO_PEEKED_FLING = 6;
    private static final int EXIT_MAXIMIZED_TO_EXPANDED_SWIPE = 7;
    private static final int EXIT_MAXIMIZED_TO_EXPANDED_FLING = 8;
    private static final int EXIT_MAXIMIZED_TO_BOUNDARY = 9;

    // Constants used to log UMA "enum" histograms with details about whether search results
    // were seen, and what the original triggering gesture was.
    private static final int RESULTS_SEEN_FROM_TAP = 0;
    private static final int RESULTS_NOT_SEEN_FROM_TAP = 1;
    private static final int RESULTS_SEEN_FROM_LONG_PRESS = 2;
    private static final int RESULTS_NOT_SEEN_FROM_LONG_PRESS = 3;
    private static final int RESULTS_BY_GESTURE_BOUNDARY = 4;

    // Constants used to log UMA "enum" histograms with details about whether search results
    // were seen, and whether any existing tap suppression heuristics were satisfied.
    private static final int RESULTS_SEEN_SUPPRESSION_HEURSTIC_SATISFIED = 0;
    private static final int RESULTS_NOT_SEEN_SUPPRESSION_HEURSTIC_SATISFIED = 1;
    private static final int RESULTS_SEEN_SUPPRESSION_HEURSTIC_NOT_SATISFIED = 2;
    private static final int RESULTS_NOT_SEEN_SUPPRESSION_HEURSTIC_NOT_SATISFIED = 3;
    private static final int RESULTS_SEEN_SUPPRESSION_BOUNDARY = 4;

    // Constants used to log UMA "enum" histograms with details about whether search results
    // were seen, and what the original triggering gesture was.
    private static final int PROMO_ENABLED_FROM_TAP = 0;
    private static final int PROMO_DISABLED_FROM_TAP = 1;
    private static final int PROMO_UNDECIDED_FROM_TAP = 2;
    private static final int PROMO_ENABLED_FROM_LONG_PRESS = 3;
    private static final int PROMO_DISABLED_FROM_LONG_PRESS = 4;
    private static final int PROMO_UNDECIDED_FROM_LONG_PRESS = 5;
    private static final int PROMO_BY_GESTURE_BOUNDARY = 6;

    // Constants used to log UMA "enum" histograms for HTTP / HTTPS.
    private static final int PROTOCOL_IS_HTTP = 0;
    private static final int PROTOCOL_NOT_HTTP = 1;
    private static final int PROTOCOL_BOUNDARY = 2;

    // Constants used to log UMA "enum" histograms for single / multi-word.
    private static final int RESOLVED_SINGLE_WORD = 0;
    private static final int RESOLVED_MULTI_WORD = 1;
    private static final int RESOLVED_BOUNDARY = 2;

    // Constants used to log UMA "enum" histograms for triggering the Translate Onebox.
    private static final int DID_FORCE_TRANSLATE = 0;
    private static final int WOULD_FORCE_TRANSLATE = 1;
    private static final int FORCE_TRANSLATE_BOUNDARY = 2;

    // Constants used to log UMA "enum" histograms for Quick Answers.
    private static final int QUICK_ANSWER_ACTIVATED_WAS_AN_ANSWER_SEEN = 0;
    private static final int QUICK_ANSWER_ACTIVATED_WAS_AN_ANSWER_NOT_SEEN = 1;
    private static final int QUICK_ANSWER_ACTIVATED_NOT_AN_ANSWER_SEEN = 2;
    private static final int QUICK_ANSWER_ACTIVATED_NOT_AN_ANSWER_NOT_SEEN = 3;
    private static final int QUICK_ANSWER_NOT_ACTIVATED_SEEN = 4;
    private static final int QUICK_ANSWER_NOT_ACTIVATED_NOT_SEEN = 5;
    private static final int QUICK_ANSWER_SEEN_BOUNDARY = 6;

    // Constants for "Bar Overlap" with triggering gesture, and whether the results were seen.
    private static final int BAR_OVERLAP_RESULTS_SEEN_FROM_TAP = 0;
    private static final int BAR_OVERLAP_RESULTS_NOT_SEEN_FROM_TAP = 1;
    private static final int NO_BAR_OVERLAP_RESULTS_SEEN_FROM_TAP = 2;
    private static final int NO_BAR_OVERLAP_RESULTS_NOT_SEEN_FROM_TAP = 3;
    private static final int BAR_OVERLAP_RESULTS_SEEN_FROM_LONG_PRESS = 4;
    private static final int BAR_OVERLAP_RESULTS_NOT_SEEN_FROM_LONG_PRESS = 5;
    private static final int NO_BAR_OVERLAP_RESULTS_SEEN_FROM_LONG_PRESS = 6;
    private static final int NO_BAR_OVERLAP_RESULTS_NOT_SEEN_FROM_LONG_PRESS = 7;
    private static final int BAR_OVERLAP_RESULTS_BOUNDARY = 8;

    // Constants for quick action intent resolution histogram.
    private static final int QUICK_ACTION_RESOLVE_FAILED = 0;
    private static final int QUICK_ACTION_RESOLVE_SINGLE = 1;
    private static final int QUICK_ACTION_RESOLVE_MULTIPLE = 2;
    private static final int QUICK_ACTION_RESOLVE_BOUNDARY = 3;

    /**
     * Key used in maps from {state, reason} to state entry (exit) logging code.
     */
    static class StateChangeKey {
        final PanelState mState;
        final StateChangeReason mReason;
        final int mHashCode;

        StateChangeKey(PanelState state, StateChangeReason reason) {
            mState = state;
            mReason = reason;
            mHashCode = 31 * state.hashCode() + reason.hashCode();
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof StateChangeKey)) {
                return false;
            }
            if (obj == this) {
                return true;
            }
            StateChangeKey other = (StateChangeKey) obj;
            return mState.equals(other.mState) && mReason.equals(other.mReason);
        }

        @Override
        public int hashCode() {
            return mHashCode;
        }
    }

    // TODO(donnd): switch from using Maps to some method that does not require creation of a key.

    // Entry code map: first entry into CLOSED.
    private static final Map<StateChangeKey, Integer> ENTER_CLOSED_STATE_CHANGE_CODES;
    static {
        Map<StateChangeKey, Integer> codes = new HashMap<StateChangeKey, Integer>();
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.BACK_PRESS),
                ENTER_CLOSED_FROM_PEEKED_BACK_PRESS);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.BASE_PAGE_SCROLL),
                ENTER_CLOSED_FROM_PEEKED_BASE_PAGE_SCROLL);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.TEXT_SELECT_TAP),
                ENTER_CLOSED_FROM_PEEKED_TEXT_SELECT_TAP);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.BACK_PRESS),
                ENTER_CLOSED_FROM_EXPANDED_BACK_PRESS);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.BASE_PAGE_TAP),
                ENTER_CLOSED_FROM_EXPANDED_BASE_PAGE_TAP);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.FLING),
                ENTER_CLOSED_FROM_EXPANDED_FLING);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.BACK_PRESS),
                ENTER_CLOSED_FROM_MAXIMIZED_BACK_PRESS);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.FLING),
                ENTER_CLOSED_FROM_MAXIMIZED_FLING);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.TAB_PROMOTION),
                ENTER_CLOSED_FROM_MAXIMIZED_TAB_PROMOTION);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.SERP_NAVIGATION),
                ENTER_CLOSED_FROM_MAXIMIZED_SERP_NAVIGATION);
        ENTER_CLOSED_STATE_CHANGE_CODES = Collections.unmodifiableMap(codes);
    }

    // Entry code map: first entry into PEEKED.
    private static final Map<StateChangeKey, Integer> ENTER_PEEKED_STATE_CHANGE_CODES;
    static {
        Map<StateChangeKey, Integer> codes = new HashMap<StateChangeKey, Integer>();
        // Note: we don't distinguish entering PEEKED from UNDEFINED / CLOSED.
        codes.put(new StateChangeKey(PanelState.UNDEFINED, StateChangeReason.TEXT_SELECT_TAP),
                ENTER_PEEKED_FROM_CLOSED_TEXT_SELECT_TAP);
        codes.put(new StateChangeKey(PanelState.UNDEFINED,
                StateChangeReason.TEXT_SELECT_LONG_PRESS),
                ENTER_PEEKED_FROM_CLOSED_EXT_SELECT_LONG_PRESS);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.TEXT_SELECT_TAP),
                ENTER_PEEKED_FROM_CLOSED_TEXT_SELECT_TAP);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.TEXT_SELECT_LONG_PRESS),
                ENTER_PEEKED_FROM_CLOSED_EXT_SELECT_LONG_PRESS);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.TEXT_SELECT_TAP),
                ENTER_PEEKED_FROM_PEEKED_TEXT_SELECT_TAP);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.TEXT_SELECT_LONG_PRESS),
                ENTER_PEEKED_FROM_PEEKED_TEXT_SELECT_LONG_PRESS);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.SEARCH_BAR_TAP),
                ENTER_PEEKED_FROM_EXPANDED_SEARCH_BAR_TAP);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.SWIPE),
                ENTER_PEEKED_FROM_EXPANDED_SWIPE);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.FLING),
                ENTER_PEEKED_FROM_EXPANDED_FLING);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.SWIPE),
                ENTER_PEEKED_FROM_MAXIMIZED_SWIPE);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.FLING),
                ENTER_PEEKED_FROM_MAXIMIZED_FLING);
        ENTER_PEEKED_STATE_CHANGE_CODES = Collections.unmodifiableMap(codes);
    }

    // Entry code map: first entry into EXPANDED.
    private static final Map<StateChangeKey, Integer> ENTER_EXPANDED_STATE_CHANGE_CODES;
    static {
        Map<StateChangeKey, Integer> codes = new HashMap<StateChangeKey, Integer>();
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.SEARCH_BAR_TAP),
                ENTER_EXPANDED_FROM_PEEKED_SEARCH_BAR_TAP);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.SWIPE),
                ENTER_EXPANDED_FROM_PEEKED_SWIPE);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.FLING),
                ENTER_EXPANDED_FROM_PEEKED_FLING);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.SWIPE),
                ENTER_EXPANDED_FROM_MAXIMIZED_SWIPE);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.FLING),
                ENTER_EXPANDED_FROM_MAXIMIZED_FLING);
        ENTER_EXPANDED_STATE_CHANGE_CODES = Collections.unmodifiableMap(codes);
    }

    // Entry code map: first entry into MAXIMIZED.
    private static final Map<StateChangeKey, Integer> ENTER_MAXIMIZED_STATE_CHANGE_CODES;
    static {
        Map<StateChangeKey, Integer> codes = new HashMap<StateChangeKey, Integer>();
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.SWIPE),
                ENTER_MAXIMIZED_FROM_PEEKED_SWIPE);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.FLING),
                ENTER_MAXIMIZED_FROM_PEEKED_FLING);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.SWIPE),
                ENTER_MAXIMIZED_FROM_EXPANDED_SWIPE);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.FLING),
                ENTER_MAXIMIZED_FROM_EXPANDED_FLING);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.SERP_NAVIGATION),
                ENTER_MAXIMIZED_FROM_EXPANDED_SERP_NAVIGATION);
        ENTER_MAXIMIZED_STATE_CHANGE_CODES = Collections.unmodifiableMap(codes);
    }

    // Exit code map: first exit from CLOSED.
    private static final Map<StateChangeKey, Integer> EXIT_CLOSED_TO_STATE_CHANGE_CODES;
    static {
        Map<StateChangeKey, Integer> codes = new HashMap<StateChangeKey, Integer>();
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.TEXT_SELECT_TAP),
                EXIT_CLOSED_TO_PEEKED_TEXT_SELECT_TAP);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.TEXT_SELECT_LONG_PRESS),
                EXIT_CLOSED_TO_PEEKED_TEXT_SELECT_LONG_PRESS);
        EXIT_CLOSED_TO_STATE_CHANGE_CODES = Collections.unmodifiableMap(codes);
    }

    // Exit code map: first exit from PEEKED.
    private static final Map<StateChangeKey, Integer> EXIT_PEEKED_TO_STATE_CHANGE_CODES;
    static {
        Map<StateChangeKey, Integer> codes = new HashMap<StateChangeKey, Integer>();
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.BACK_PRESS),
                EXIT_PEEKED_TO_CLOSED_BACK_PRESS);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.BASE_PAGE_SCROLL),
                EXIT_PEEKED_TO_CLOSED_BASE_PAGE_SCROLL);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.BASE_PAGE_TAP),
                EXIT_PEEKED_TO_CLOSED_TEXT_SELECT_TAP);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.TEXT_SELECT_TAP),
                EXIT_PEEKED_TO_PEEKED_TEXT_SELECT_TAP);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.TEXT_SELECT_LONG_PRESS),
                EXIT_PEEKED_TO_PEEKED_TEXT_SELECT_LONG_PRESS);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.SEARCH_BAR_TAP),
                EXIT_PEEKED_TO_EXPANDED_SEARCH_BAR_TAP);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.SWIPE),
                EXIT_PEEKED_TO_EXPANDED_SWIPE);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.FLING),
                EXIT_PEEKED_TO_EXPANDED_FLING);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.SWIPE),
                EXIT_PEEKED_TO_MAXIMIZED_SWIPE);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.FLING),
                EXIT_PEEKED_TO_MAXIMIZED_FLING);
        EXIT_PEEKED_TO_STATE_CHANGE_CODES = Collections.unmodifiableMap(codes);
    }

    // Exit code map: first exit from EXPANDED.
    private static final Map<StateChangeKey, Integer> EXIT_EXPANDED_TO_STATE_CHANGE_CODES;
    static {
        Map<StateChangeKey, Integer> codes = new HashMap<StateChangeKey, Integer>();
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.BACK_PRESS),
                EXIT_EXPANDED_TO_CLOSED_BACK_PRESS);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.BASE_PAGE_TAP),
                EXIT_EXPANDED_TO_CLOSED_BASE_PAGE_TAP);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.FLING),
                EXIT_EXPANDED_TO_CLOSED_FLING);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.SEARCH_BAR_TAP),
                EXIT_EXPANDED_TO_PEEKED_SEARCH_BAR_TAP);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.SWIPE),
                EXIT_EXPANDED_TO_PEEKED_SWIPE);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.FLING),
                EXIT_EXPANDED_TO_PEEKED_FLING);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.SWIPE),
                EXIT_EXPANDED_TO_MAXIMIZED_SWIPE);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.FLING),
                EXIT_EXPANDED_TO_MAXIMIZED_FLING);
        codes.put(new StateChangeKey(PanelState.MAXIMIZED, StateChangeReason.SERP_NAVIGATION),
                EXIT_EXPANDED_TO_MAXIMIZED_SERP_NAVIGATION);
        EXIT_EXPANDED_TO_STATE_CHANGE_CODES = Collections.unmodifiableMap(codes);
    }

    // Exit code map: first exit from MAXIMIZED.
    private static final Map<StateChangeKey, Integer> EXIT_MAXIMIZED_TO_STATE_CHANGE_CODES;
    static {
        Map<StateChangeKey, Integer> codes = new HashMap<StateChangeKey, Integer>();
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.BACK_PRESS),
                EXIT_MAXIMIZED_TO_CLOSED_BACK_PRESS);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.FLING),
                EXIT_MAXIMIZED_TO_CLOSED_FLING);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.TAB_PROMOTION),
                EXIT_MAXIMIZED_TO_CLOSED_TAB_PROMOTION);
        codes.put(new StateChangeKey(PanelState.CLOSED, StateChangeReason.SERP_NAVIGATION),
                EXIT_MAXIMIZED_TO_CLOSED_SERP_NAVIGATION);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.SWIPE),
                EXIT_MAXIMIZED_TO_PEEKED_SWIPE);
        codes.put(new StateChangeKey(PanelState.PEEKED, StateChangeReason.FLING),
                EXIT_MAXIMIZED_TO_PEEKED_FLING);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.SWIPE),
                EXIT_MAXIMIZED_TO_EXPANDED_SWIPE);
        codes.put(new StateChangeKey(PanelState.EXPANDED, StateChangeReason.FLING),
                EXIT_MAXIMIZED_TO_EXPANDED_FLING);
        EXIT_MAXIMIZED_TO_STATE_CHANGE_CODES = Collections.unmodifiableMap(codes);
    }

    // "Seen by gesture" code map: logged on first exit from expanded panel, or promo,
    // broken down by gesture.
    private static final Map<Pair<Boolean, Boolean>, Integer> SEEN_BY_GESTURE_CODES;
    static {
        final boolean unseen = false;
        final boolean seen = true;
        Map<Pair<Boolean, Boolean>, Integer> codes = new HashMap<Pair<Boolean, Boolean>, Integer>();
        codes.put(new Pair<Boolean, Boolean>(seen, TAP), RESULTS_SEEN_FROM_TAP);
        codes.put(new Pair<Boolean, Boolean>(unseen, TAP), RESULTS_NOT_SEEN_FROM_TAP);
        codes.put(new Pair<Boolean, Boolean>(seen, LONG_PRESS), RESULTS_SEEN_FROM_LONG_PRESS);
        codes.put(new Pair<Boolean, Boolean>(unseen, LONG_PRESS), RESULTS_NOT_SEEN_FROM_LONG_PRESS);
        SEEN_BY_GESTURE_CODES = Collections.unmodifiableMap(codes);
    }

    // "Promo outcome by gesture" code map: logged on exit from promo, broken down by gesture.
    private static final Map<Pair<Integer, Boolean>, Integer> PROMO_BY_GESTURE_CODES;
    static {
        Map<Pair<Integer, Boolean>, Integer> codes =
                new HashMap<Pair<Integer, Boolean>, Integer>();
        codes.put(new Pair<Integer, Boolean>(PREFERENCE_ENABLED, TAP), PROMO_ENABLED_FROM_TAP);
        codes.put(new Pair<Integer, Boolean>(PREFERENCE_DISABLED, TAP), PROMO_DISABLED_FROM_TAP);
        codes.put(new Pair<Integer, Boolean>(PREFERENCE_UNINITIALIZED, TAP),
                PROMO_UNDECIDED_FROM_TAP);
        codes.put(new Pair<Integer, Boolean>(PREFERENCE_ENABLED, LONG_PRESS),
                PROMO_ENABLED_FROM_LONG_PRESS);
        codes.put(new Pair<Integer, Boolean>(PREFERENCE_DISABLED, LONG_PRESS),
                PROMO_DISABLED_FROM_LONG_PRESS);
        codes.put(new Pair<Integer, Boolean>(PREFERENCE_UNINITIALIZED, LONG_PRESS),
                PROMO_UNDECIDED_FROM_LONG_PRESS);
        PROMO_BY_GESTURE_CODES = Collections.unmodifiableMap(codes);
    }

    /**
     * Logs the state of the Contextual Search preference. This function should be called if the
     * Contextual Search feature is active, and will track the different preference settings
     * (disabled, enabled or uninitialized). Calling more than once is fine.
     */
    public static void logPreferenceState() {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchPreferenceState",
                getPreferenceValue(), PREFERENCE_HISTOGRAM_BOUNDARY);
    }

    /**
     * Logs the given number of promo taps remaining.  Should be called only for users that
     * are still undecided.
     * @param promoTapsRemaining The number of taps remaining (should not be negative).
     */
    public static void logPromoTapsRemaining(int promoTapsRemaining) {
        if (promoTapsRemaining >= 0) {
            RecordHistogram.recordCountHistogram("Search.ContextualSearchPromoTapsRemaining",
                    promoTapsRemaining);
        }
    }

    /**
     * Logs the historic number of times that a Tap gesture triggered the peeking promo
     * for users that have never opened the panel.  This should be called periodically for
     * undecided users only.
     * @param promoTaps The historic number of taps that have caused the peeking bar for the promo,
     *        for users that have never opened the panel.
     */
    public static void logPromoTapsForNeverOpened(int promoTaps) {
        RecordHistogram.recordCountHistogram("Search.ContextualSearchPromoTapsForNeverOpened",
                promoTaps);
    }

    /**
     * Logs the historic number of times that a Tap gesture triggered the peeking promo before
     * the user ever opened the panel.  This should be called periodically for all users.
     * @param promoTaps The historic number of taps that have caused the peeking bar for the promo
     *        before the first open of the panel, for all users that have ever opened the panel.
     */
    public static void logPromoTapsBeforeFirstOpen(int promoTaps) {
        RecordHistogram.recordCountHistogram("Search.ContextualSearchPromoTapsBeforeFirstOpen",
                promoTaps);
    }

    /**
     * Records the total count of times the promo panel has *ever* been opened.  This should only
     * be called when the user is still undecided.
     * @param count The total historic count of times the panel has ever been opened for the
     *        current user.
     */
    public static void logPromoOpenCount(int count) {
        RecordHistogram.recordCountHistogram("Search.ContextualSearchPromoOpenCount", count);
    }

    /**
     * Logs the number of taps that have been counted since the user last opened the panel, for
     * undecided users.
     * @param tapsSinceOpen The number of taps to log.
     */
    public static void logTapsSinceOpenForUndecided(int tapsSinceOpen) {
        RecordHistogram.recordCountHistogram("Search.ContextualSearchTapsSinceOpenUndecided",
                tapsSinceOpen);
    }

    /**
     * Logs the number of taps that have been counted since the user last opened the panel, for
     * decided users.
     * @param tapsSinceOpen The number of taps to log.
     */
    public static void logTapsSinceOpenForDecided(int tapsSinceOpen) {
        RecordHistogram.recordCountHistogram("Search.ContextualSearchTapsSinceOpenDecided",
                tapsSinceOpen);
    }

    /**
     * Logs whether the Search Term was single or multiword.
     * @param isSingleWord Whether the resolved search term is a single word or not.
     */
    public static void logSearchTermResolvedWords(boolean isSingleWord) {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchResolvedTermWords",
                isSingleWord ? RESOLVED_SINGLE_WORD : RESOLVED_MULTI_WORD, RESOLVED_BOUNDARY);
    }

    /**
     * Logs whether the base page was using the HTTP protocol or not.
     * @param isHttpBasePage Whether the base page was using the HTTP protocol or not (should
     *        be false for HTTPS or other URIs).
     */
    public static void logBasePageProtocol(boolean isHttpBasePage) {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchBasePageProtocol",
                isHttpBasePage ? PROTOCOL_IS_HTTP : PROTOCOL_NOT_HTTP, PROTOCOL_BOUNDARY);
    }

    /**
     * Logs changes to the Contextual Search preference, aside from those resulting from the first
     * run flow.
     * @param enabled Whether the preference is being enabled or disabled.
     */
    public static void logPreferenceChange(boolean enabled) {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchPreferenceStateChange",
                enabled ? PREFERENCE_ENABLED : PREFERENCE_DISABLED, PREFERENCE_HISTOGRAM_BOUNDARY);
    }

    /**
     * Logs the outcome of the Promo.
     * Logs multiple histograms; with and without the originating gesture.
     * @param wasTap Whether the gesture that originally caused the panel to show was a Tap.
     * @param wasMandatory Whether the Promo was mandatory.
     */
    public static void logPromoOutcome(boolean wasTap, boolean wasMandatory) {
        int preferenceCode = getPreferenceValue();
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchFirstRunFlowOutcome",
                preferenceCode, PREFERENCE_HISTOGRAM_BOUNDARY);

        int preferenceByGestureCode = getPromoByGestureStateCode(preferenceCode, wasTap);
        if (wasMandatory) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Search.ContextualSearchMandatoryPromoOutcomeByGesture",
                    preferenceByGestureCode, PROMO_BY_GESTURE_BOUNDARY);
        } else {
            RecordHistogram.recordEnumeratedHistogram(
                    "Search.ContextualSearchPromoOutcomeByGesture",
                    preferenceByGestureCode, PROMO_BY_GESTURE_BOUNDARY);
        }
    }

    /**
     * Logs the duration of a Contextual Search panel being viewed by the user.
     * @param wereResultsSeen Whether search results were seen.
     * @param isChained Whether the Contextual Search ended with the start of another.
     * @param durationMs The duration of the contextual search in milliseconds.
     */
    public static void logDuration(boolean wereResultsSeen, boolean isChained, long durationMs) {
        if (wereResultsSeen) {
            RecordHistogram.recordTimesHistogram("Search.ContextualSearchDurationSeen",
                    durationMs, TimeUnit.MILLISECONDS);
        } else if (isChained) {
            RecordHistogram.recordTimesHistogram("Search.ContextualSearchDurationUnseenChained",
                    durationMs, TimeUnit.MILLISECONDS);
        } else {
            RecordHistogram.recordTimesHistogram("Search.ContextualSearchDurationUnseen",
                    durationMs, TimeUnit.MILLISECONDS);
        }
    }

    /**
     * Logs the duration from starting a search until the Search Term is resolved.
     * @param durationMs The duration to record.
     */
    public static void logSearchTermResolutionDuration(long durationMs) {
        RecordHistogram.recordMediumTimesHistogram(
                "Search.ContextualSearchResolutionDuration", durationMs, TimeUnit.MILLISECONDS);
    }

    /**
     * Logs the duration from starting a prefetched search until the panel navigates to the results
     * and they start becoming viewable. Should be called only for searches that are prefetched.
     * @param durationMs The duration to record.
     * @param didResolve Whether a Search Term resolution was required as part of the loading.
     */
    public static void logPrefetchedSearchNavigatedDuration(long durationMs, boolean didResolve) {
        String histogramName = didResolve ? "Search.ContextualSearchResolvedSearchDuration"
                                          : "Search.ContextualSearchLiteralSearchDuration";
        RecordHistogram.recordMediumTimesHistogram(
                histogramName, durationMs, TimeUnit.MILLISECONDS);
    }

    /**
     * Logs the duration from opening the panel beyond peek until the panel is closed.
     * @param durationMs The duration to record.
     */
    public static void logPanelOpenDuration(long durationMs) {
        RecordHistogram.recordMediumTimesHistogram(
                "Search.ContextualSearchPanelOpenDuration", durationMs, TimeUnit.MILLISECONDS);
    }

    /**
     * When Contextual Search panel is opened, logs whether In-Product Help for opening the panel
     * was ever shown.
     * @param wasIPHShown Whether In-Product help was shown.
     */
    public static void logPanelOpenedIPH(boolean wasIPHShown) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchPanelOpenedIPHShown", wasIPHShown);
    }

    /**
     * When Contextual Search panel is opened, logs whether In-Product Help for Contextual Search
     * was ever shown.
     * @param wasIPHShown Whether In-Product help was shown.
     */
    public static void logContextualSearchIPH(boolean wasIPHShown) {
        RecordHistogram.recordBooleanHistogram("Search.ContextualSearchIPHShown", wasIPHShown);
    }

    /**
     * When Contextual Search is triggered by tapping, logs whether In-Product Help for tapping was
     * ever shown.
     * @param wasIPHShown Whether In-Product help was shown.
     */
    public static void logTapIPH(boolean wasIPHShown) {
        RecordHistogram.recordBooleanHistogram("Search.ContextualSearchTapIPHShown", wasIPHShown);
    }

    /**
     * Logs a user action for the duration of viewing the panel that describes the amount of time
     * the user viewed the bar and panel overall.
     * @param durationMs The duration to record.
     */
    public static void logPanelViewDurationAction(long durationMs) {
        if (durationMs < 1000) {
            RecordUserAction.record("ContextualSearch.ViewLessThanOneSecond");
        } else if (durationMs < 3000) {
            RecordUserAction.record("ContextualSearch.ViewOneToThreeSeconds");
        } else if (durationMs < 10000) {
            RecordUserAction.record("ContextualSearch.ViewThreeToTenSeconds");
        } else {
            RecordUserAction.record("ContextualSearch.ViewMoreThanTenSeconds");
        }
    }

    /**
     * Logs whether the promo was seen.
     * Logs multiple histograms, with and without the original triggering gesture.
     * @param wasPanelSeen Whether the panel was seen.
     * @param wasTap Whether the gesture that originally caused the panel to show was a Tap.
     */
    public static void logPromoSeen(boolean wasPanelSeen, boolean wasTap) {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchFirstRunPanelSeen",
                wasPanelSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
        logHistogramByGesture(wasPanelSeen, wasTap, "Search.ContextualSearchPromoSeenByGesture");
    }

    /**
     * Logs whether search results were seen.
     * Logs multiple histograms; with and without the original triggering gesture.
     * @param wasPanelSeen Whether the panel was seen.
     * @param wasTap Whether the gesture that originally caused the panel to show was a Tap.
     */
    public static void logResultsSeen(boolean wasPanelSeen, boolean wasTap) {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchResultsSeen",
                wasPanelSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
        logHistogramByGesture(wasPanelSeen, wasTap, "Search.ContextualSearchResultsSeenByGesture");
    }

    /**
     * Logs whether search results were seen for a Tap gesture.  Recorded for all users.
     * @param wasPanelSeen Whether the panel was seen.
     */
    public static void logTapResultsSeen(boolean wasPanelSeen) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearch.Tap.ResultsSeen", wasPanelSeen);
    }

    /**
     * Logs whether search results were seen for all gestures.  Recorded for all users.
     * @param wasPanelSeen Whether the panel was seen.
     */
    public static void logAllResultsSeen(boolean wasPanelSeen) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearch.All.ResultsSeen", wasPanelSeen);
    }

    /**
     * Logs the whether the panel was seen and the type of the trigger and if Bar nearly overlapped.
     * If the panel was seen, logs the duration of the panel view into a BarOverlap or BarNoOverlap
     * duration histogram.
     * @param wasPanelSeen Whether the panel was seen.
     * @param wasTap Whether the gesture was a Tap or not.
     * @param wasBarOverlap Whether the trigger location overlapped the Bar area.
     */
    public static void logBarOverlapResultsSeen(
            boolean wasPanelSeen, boolean wasTap, boolean wasBarOverlap) {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchBarOverlapSeen",
                getBarOverlapEnum(wasBarOverlap, wasPanelSeen, wasTap),
                BAR_OVERLAP_RESULTS_BOUNDARY);
    }

    /**
     * Logs the duration of the panel viewed in its Peeked state before being opened.
     * @param wasBarOverlap Whether the trigger location overlapped the Bar area.
     * @param panelPeekDurationMs The duration that the panel was peeking before being opened
     *        by the user.
     */
    public static void logBarOverlapPeekDuration(boolean wasBarOverlap, long panelPeekDurationMs) {
        String histogram = wasBarOverlap ? "Search.ContextualSearchBarOverlap.PeekDuration"
                                         : "Search.ContextualSearchBarNoOverlap.PeekDuration";
        RecordHistogram.recordMediumTimesHistogram(
                histogram, panelPeekDurationMs, TimeUnit.MILLISECONDS);
    }

    /**
     * Log whether the UX was suppressed due to Bar overlap.
     * @param wasSuppressed Whether showing the UX was suppressed.
     */
    public static void logBarOverlapSuppression(boolean wasSuppressed) {
        RecordHistogram.recordBooleanHistogram("Search.ContextualSearchBarOverlap", wasSuppressed);
    }

    /**
     * Logs the length of the selection in two histograms, one when results were seen and one when
     * results were not seen.
     * @param wasPanelSeen Whether the panel was seen.
     * @param selectionLength The length of the triggering selection.
     */
    public static void logSelectionLengthResultsSeen(boolean wasPanelSeen, int selectionLength) {
        if (wasPanelSeen) {
            RecordHistogram.recordSparseSlowlyHistogram(
                    "Search.ContextualSearchSelectionLengthSeen", selectionLength);
        } else {
            RecordHistogram.recordSparseSlowlyHistogram(
                    "Search.ContextualSearchSelectionLengthNotSeen", selectionLength);
        }
    }

    /**
     * Log whether the UX was suppressed due to the selection length.
     * @param wasSuppressed Whether showing the UX was suppressed due to selection length.
     */
    public static void logSelectionLengthSuppression(boolean wasSuppressed) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchSelectionLengthSuppression", wasSuppressed);
    }

    /**
     * Logs the location of a Tap and whether the panel was seen and the type of the
     * trigger.
     * @param wasPanelSeen Whether the panel was seen.
     * @param wasTap Whether the gesture was a Tap or not.
     * @param triggerLocationDps The trigger location from the top of the screen.
     */
    public static void logScreenTopTapLocation(
            boolean wasPanelSeen, boolean wasTap, int triggerLocationDps) {
        // We only log Tap locations for the screen top.
        if (!wasTap) return;
        String histogram = wasPanelSeen ? "Search.ContextualSearchTopLocationSeen"
                                        : "Search.ContextualSearchTopLocationNotSeen";
        int min = 1;
        int max = 250;
        int numBuckets = 50;
        RecordHistogram.recordCustomCountHistogram(
                histogram, triggerLocationDps, min, max, numBuckets);
    }

    /**
     * Log whether the UX was suppressed due to a Tap too close to the screen top.
     * @param wasSuppressed Whether showing the UX was suppressed.
     */
    public static void logScreenTopTapSuppression(boolean wasSuppressed) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchScreenTopSuppressed", wasSuppressed);
    }

    /**
     * Log whether results were seen due to a Tap that was allowed to override an ML suppression.
     * @param wasSearchContentViewSeen If the panel was opened.
     */
    static void logSecondTapMlOverrideResultsSeen(boolean wasSearchContentViewSeen) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchSecondTapMlOverrideSeen", wasSearchContentViewSeen);
    }

    /**
     * Logs whether results were seen based on the duration of the Tap, for both short and long
     * durations.
     * @param wasSearchContentViewSeen If the panel was opened.
     * @param isTapShort Whether this tap was "short" in duration.
     */
    public static void logTapDurationSeen(boolean wasSearchContentViewSeen, boolean isTapShort) {
        if (isTapShort) {
            RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchTapShortDurationSeen",
                    wasSearchContentViewSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN,
                    RESULTS_SEEN_BOUNDARY);
        } else {
            RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchTapLongDurationSeen",
                    wasSearchContentViewSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN,
                    RESULTS_SEEN_BOUNDARY);
        }
    }

    /**
     * Logs the duration of a Tap in ms into custom histograms to profile the duration of seen
     * and not seen taps.
     * @param wasPanelSeen Whether the panel was seen.
     * @param durationMs The duration of the tap gesture.
     */
    public static void logTapDuration(boolean wasPanelSeen, int durationMs) {
        int min = 1;
        int max = 1000;
        int numBuckets = 100;

        if (wasPanelSeen) {
            RecordHistogram.recordCustomCountHistogram(
                    "Search.ContextualSearchTapDurationSeen", durationMs, min, max, numBuckets);
        } else {
            RecordHistogram.recordCustomCountHistogram(
                    "Search.ContextualSearchTapDurationNotSeen", durationMs, min, max, numBuckets);
        }
    }

    /**
     * Log whether results were seen due to a Tap on a short word.
     * @param wasSearchContentViewSeen If the panel was opened.
     * @param isTapOnShortWord Whether this tap was on a "short" word.
     */
    public static void logTapShortWordSeen(
            boolean wasSearchContentViewSeen, boolean isTapOnShortWord) {
        if (!isTapOnShortWord) return;

        // We just record CTR of short words.
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchTapShortWordSeen",
                wasSearchContentViewSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
    }

    /**
     * Log whether results were seen due to a Tap on a long word.
     * @param wasSearchContentViewSeen If the panel was opened.
     * @param isTapOnLongWord Whether this tap was on a long word.
     */
    public static void logTapLongWordSeen(
            boolean wasSearchContentViewSeen, boolean isTapOnLongWord) {
        if (!isTapOnLongWord) return;

        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchTapLongWordSeen",
                wasSearchContentViewSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
    }

    /**
     * Log whether results were seen due to a Tap that was on the middle of a word.
     * @param wasSearchContentViewSeen If the panel was opened.
     * @param isTapOnWordMiddle Whether this tap was on the middle of a word.
     */
    public static void logTapOnWordMiddleSeen(
            boolean wasSearchContentViewSeen, boolean isTapOnWordMiddle) {
        if (!isTapOnWordMiddle) return;

        // We just record CTR of words tapped in the "middle".
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchTapOnWordMiddleSeen",
                wasSearchContentViewSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
    }

    /**
     * Log whether results were seen due to a Tap on what we've recognized as a probable entity.
     * @param wasSearchContentViewSeen If the panel was opened.
     * @param isWordAnEntity Whether this tap was on a word that's an entity.
     */
    public static void logTapOnEntitySeen(
            boolean wasSearchContentViewSeen, boolean isWordAnEntity) {
        if (isWordAnEntity) {
            // We just record CTR of probable entity words.
            RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchEntitySeen",
                    wasSearchContentViewSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN,
                    RESULTS_SEEN_BOUNDARY);
        }
    }

    /**
     * Logs whether results were seen and whether any tap suppression heuristics were satisfied.
     * @param wasSearchContentViewSeen If the panel was opened.
     * @param wasAnySuppressionHeuristicSatisfied Whether any of the implemented suppression
     *                                            heuristics were satisfied.
     */
    public static void logAnyTapSuppressionHeuristicSatisfied(boolean wasSearchContentViewSeen,
            boolean wasAnySuppressionHeuristicSatisfied) {
        int code;
        if (wasAnySuppressionHeuristicSatisfied) {
            code = wasSearchContentViewSeen ? RESULTS_SEEN_SUPPRESSION_HEURSTIC_SATISFIED
                    : RESULTS_NOT_SEEN_SUPPRESSION_HEURSTIC_SATISFIED;
        } else {
            code = wasSearchContentViewSeen ? RESULTS_SEEN_SUPPRESSION_HEURSTIC_NOT_SATISFIED
                    : RESULTS_NOT_SEEN_SUPPRESSION_HEURSTIC_NOT_SATISFIED;
        }

        RecordHistogram.recordEnumeratedHistogram(
                "Search.ContextualSearchTapSuppressionSeen.AnyHeuristicSatisfied",
                code,
                RESULTS_SEEN_SUPPRESSION_BOUNDARY);
    }

    /**
     * Logs whether a selection is valid.
     * @param isSelectionValid Whether the selection is valid.
     */
    public static void logSelectionIsValid(boolean isSelectionValid) {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchSelectionValid",
                isSelectionValid ? SELECTION_VALID : SELECTION_INVALID, SELECTION_BOUNDARY);
    }

    /**
     * Logs whether a normal priority search request failed.
     * @param isFailure Whether the request failed.
     */
    public static void logNormalPrioritySearchRequestOutcome(boolean isFailure) {
        RecordHistogram.recordEnumeratedHistogram(
                "Search.ContextualSearchNormalPrioritySearchRequestStatus",
                isFailure ? REQUEST_FAILED : REQUEST_NOT_FAILED, REQUEST_BOUNDARY);
    }

    /**
     * Logs whether a low priority search request failed.
     * @param isFailure Whether the request failed.
     */
    public static void logLowPrioritySearchRequestOutcome(boolean isFailure) {
        RecordHistogram.recordEnumeratedHistogram(
                "Search.ContextualSearchLowPrioritySearchRequestStatus",
                isFailure ? REQUEST_FAILED : REQUEST_NOT_FAILED, REQUEST_BOUNDARY);
    }

    /**
     * Logs whether a fallback search request failed.
     * @param isFailure Whether the request failed.
     */
    public static void logFallbackSearchRequestOutcome(boolean isFailure) {
        RecordHistogram.recordEnumeratedHistogram(
                "Search.ContextualSearchFallbackSearchRequestStatus",
                isFailure ? REQUEST_FAILED : REQUEST_NOT_FAILED, REQUEST_BOUNDARY);
    }

    /**
     * Log whether the UX was suppressed by a recent scroll.
     * @param wasSuppressed Whether showing the UX was suppressed by a recent scroll.
     */
    public static void logRecentScrollSuppression(boolean wasSuppressed) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchRecentScrollSuppression", wasSuppressed);
    }

    /**
     * Logs the duration between the panel being triggered due to a tap and the panel being
     * dismissed due to a scroll.
     * @param durationSincePanelTriggerMs The amount of time between the panel getting triggered and
     *                                    the panel being dismissed due to a scroll.
     * @param wasSearchContentViewSeen If the panel was opened.
     */
    public static void logDurationBetweenTriggerAndScroll(
            long durationSincePanelTriggerMs, boolean wasSearchContentViewSeen) {
        String histogram = wasSearchContentViewSeen
                ? "Search.ContextualSearchDurationBetweenTriggerAndScrollSeen"
                : "Search.ContextualSearchDurationBetweenTriggerAndScrollNotSeen";
        if (durationSincePanelTriggerMs < 2000) {
            RecordHistogram.recordCustomCountHistogram(
                    histogram, (int) durationSincePanelTriggerMs, 1, 2000, 200);
        }
    }

    /**
     * Logs whether a Quick Answer caption was activated, and whether it was an answer (as opposed
     * to just being informative), and whether the panel was opened anyway.
     * Logged only for Tap events.
     * @param didActivate If the Quick Answer caption was shown.
     * @param didAnswer If the caption was considered an answer (reducing the need to open the
     *        panel).
     * @param wasSearchContentViewSeen If the panel was opened.
     */
    static void logQuickAnswerSeen(
            boolean wasSearchContentViewSeen, boolean didActivate, boolean didAnswer) {
        RecordHistogram.recordEnumeratedHistogram("Search.ContextualSearchQuickAnswerSeen",
                getQuickAnswerSeenValue(didActivate, didAnswer, wasSearchContentViewSeen),
                QUICK_ANSWER_SEEN_BOUNDARY);
    }

    /**
     * Logs how a state was entered for the first time within a Contextual Search.
     * @param fromState The state to transition from.
     * @param toState The state to transition to.
     * @param reason The reason for the state transition.
     */
    public static void logFirstStateEntry(PanelState fromState, PanelState toState,
            StateChangeReason reason) {
        int code;
        switch (toState) {
            case CLOSED:
                code = getStateChangeCode(fromState, reason,
                        ENTER_CLOSED_STATE_CHANGE_CODES, ENTER_CLOSED_FROM_OTHER);
                RecordHistogram.recordEnumeratedHistogram(
                        "Search.ContextualSearchEnterClosed",
                        code, ENTER_CLOSED_FROM_BOUNDARY);
                break;
            case PEEKED:
                code = getStateChangeCode(fromState, reason,
                        ENTER_PEEKED_STATE_CHANGE_CODES, ENTER_PEEKED_FROM_OTHER);
                RecordHistogram.recordEnumeratedHistogram(
                        "Search.ContextualSearchEnterPeeked",
                        code, ENTER_PEEKED_FROM_BOUNDARY);
                break;
            case EXPANDED:
                code = getStateChangeCode(fromState, reason,
                        ENTER_EXPANDED_STATE_CHANGE_CODES, ENTER_EXPANDED_FROM_OTHER);
                RecordHistogram.recordEnumeratedHistogram(
                        "Search.ContextualSearchEnterExpanded",
                        code, ENTER_EXPANDED_FROM_BOUNDARY);
                break;
            case MAXIMIZED:
                code = getStateChangeCode(fromState, reason,
                        ENTER_MAXIMIZED_STATE_CHANGE_CODES, ENTER_MAXIMIZED_FROM_OTHER);
                RecordHistogram.recordEnumeratedHistogram(
                        "Search.ContextualSearchEnterMaximized",
                        code, ENTER_MAXIMIZED_FROM_BOUNDARY);
                break;
            default:
                break;
        }
    }

    /**
     * Logs a user action for a change to the Panel state, which allows sequencing of actions.
     * @param toState The state to transition to.
     * @param reason The reason for the state transition.
     */
    public static void logPanelStateUserAction(PanelState toState, StateChangeReason reason) {
        switch (toState) {
            case CLOSED:
                if (reason == StateChangeReason.BACK_PRESS) {
                    RecordUserAction.record("ContextualSearch.BackPressClose");
                } else if (reason == StateChangeReason.CLOSE_BUTTON) {
                    RecordUserAction.record("ContextualSearch.CloseButtonClose");
                } else if (reason == StateChangeReason.SWIPE || reason == StateChangeReason.FLING) {
                    RecordUserAction.record("ContextualSearch.SwipeOrFlingClose");
                } else if (reason == StateChangeReason.TAB_PROMOTION) {
                    RecordUserAction.record("ContextualSearch.TabPromotionClose");
                } else if (reason == StateChangeReason.BASE_PAGE_TAP) {
                    RecordUserAction.record("ContextualSearch.BasePageTapClose");
                } else if (reason == StateChangeReason.BASE_PAGE_SCROLL) {
                    RecordUserAction.record("ContextualSearch.BasePageScrollClose");
                } else if (reason == StateChangeReason.SEARCH_BAR_TAP) {
                    RecordUserAction.record("ContextualSearch.SearchBarTapClose");
                } else if (reason == StateChangeReason.SERP_NAVIGATION) {
                    RecordUserAction.record("ContextualSearch.NavigationClose");
                } else {
                    RecordUserAction.record("ContextualSearch.UncommonClose");
                }
                break;
            case PEEKED:
                if (reason == StateChangeReason.TEXT_SELECT_TAP) {
                    RecordUserAction.record("ContextualSearch.TapPeek");
                } else if (reason == StateChangeReason.SWIPE || reason == StateChangeReason.FLING) {
                    RecordUserAction.record("ContextualSearch.SwipeOrFlingPeek");
                } else if (reason == StateChangeReason.TEXT_SELECT_LONG_PRESS) {
                    RecordUserAction.record("ContextualSearch.LongpressPeek");
                }
                break;
            case EXPANDED:
                if (reason == StateChangeReason.SWIPE || reason == StateChangeReason.FLING) {
                    RecordUserAction.record("ContextualSearch.SwipeOrFlingExpand");
                } else if (reason == StateChangeReason.SEARCH_BAR_TAP) {
                    RecordUserAction.record("ContextualSearch.SearchBarTapExpand");
                }
                break;
            case MAXIMIZED:
                if (reason == StateChangeReason.SWIPE || reason == StateChangeReason.FLING) {
                    RecordUserAction.record("ContextualSearch.SwipeOrFlingMaximize");
                } else if (reason == StateChangeReason.SERP_NAVIGATION) {
                    RecordUserAction.record("ContextualSearch.NavigationMaximize");
                }
                break;
            default:
                break;
        }
    }

    /**
     * Logs how a state was exited for the first time within a Contextual Search.
     * @param fromState The state to transition from.
     * @param toState The state to transition to.
     * @param reason The reason for the state transition.
     */
    public static void logFirstStateExit(PanelState fromState, PanelState toState,
            StateChangeReason reason) {
        int code;
        switch (fromState) {
            case UNDEFINED:
            case CLOSED:
                code = getStateChangeCode(toState, reason,
                        EXIT_CLOSED_TO_STATE_CHANGE_CODES, EXIT_CLOSED_TO_OTHER);
                RecordHistogram.recordEnumeratedHistogram(
                        "Search.ContextualSearchExitClosed", code, EXIT_CLOSED_TO_BOUNDARY);
                break;
            case PEEKED:
                code = getStateChangeCode(toState, reason,
                        EXIT_PEEKED_TO_STATE_CHANGE_CODES, EXIT_PEEKED_TO_OTHER);
                RecordHistogram.recordEnumeratedHistogram(
                        "Search.ContextualSearchExitPeeked", code, EXIT_PEEKED_TO_BOUNDARY);
                break;
            case EXPANDED:
                code = getStateChangeCode(toState, reason,
                        EXIT_EXPANDED_TO_STATE_CHANGE_CODES, EXIT_EXPANDED_TO_OTHER);
                RecordHistogram.recordEnumeratedHistogram(
                        "Search.ContextualSearchExitExpanded", code, EXIT_EXPANDED_TO_BOUNDARY);
                break;
            case MAXIMIZED:
                code = getStateChangeCode(toState, reason,
                        EXIT_MAXIMIZED_TO_STATE_CHANGE_CODES, EXIT_MAXIMIZED_TO_OTHER);
                RecordHistogram.recordEnumeratedHistogram(
                        "Search.ContextualSearchExitMaximized", code, EXIT_MAXIMIZED_TO_BOUNDARY);
                break;
            default:
                break;
        }
    }

    /**
     * Logs the number of impressions and CTR for the previous week for the current user.
     * @param previousWeekImpressions The number of times the user saw the Contextual Search Bar.
     * @param previousWeekCtr The CTR expressed as a percentage.
     */
    public static void logPreviousWeekCtr(int previousWeekImpressions, int previousWeekCtr) {
        RecordHistogram.recordCountHistogram(
                "Search.ContextualSearchPreviousWeekImpressions", previousWeekImpressions);
        RecordHistogram.recordPercentageHistogram(
                "Search.ContextualSearchPreviousWeekCtr", previousWeekCtr);
    }

    /**
     * Logs the number of impressions and CTR for previous 28-day period for the current user.
     * @param previous28DayImpressions The number of times the user saw the Contextual Search Bar.
     * @param previous28DayCtr The CTR expressed as a percentage.
     */
    public static void logPrevious28DayCtr(int previous28DayImpressions, int previous28DayCtr) {
        RecordHistogram.recordCountHistogram(
                "Search.ContextualSearchPrevious28DayImpressions", previous28DayImpressions);
        RecordHistogram.recordPercentageHistogram(
                "Search.ContextualSearchPrevious28DayCtr", previous28DayCtr);
    }

    /**
     * Get the encoded value to use for the Bar Overlap histogram by encoding all the input
     * parameters.
     * @param didBarOverlap Whether the selection overlapped the Bar position.
     * @param wasPanelSeen Whether the panel content was seen.
     * @param wasTap Whether the gesture was a Tap.
     * @return The value for the enum histogram.
     */
    private static int getBarOverlapEnum(
            boolean didBarOverlap, boolean wasPanelSeen, boolean wasTap) {
        if (wasTap) {
            if (didBarOverlap) {
                if (wasPanelSeen) {
                    return BAR_OVERLAP_RESULTS_SEEN_FROM_TAP;
                } else {
                    return BAR_OVERLAP_RESULTS_NOT_SEEN_FROM_TAP;
                }
            } else {
                if (wasPanelSeen) {
                    return NO_BAR_OVERLAP_RESULTS_SEEN_FROM_TAP;
                } else {
                    return NO_BAR_OVERLAP_RESULTS_NOT_SEEN_FROM_TAP;
                }
            }
        } else {
            if (didBarOverlap) {
                if (wasPanelSeen) {
                    return BAR_OVERLAP_RESULTS_SEEN_FROM_LONG_PRESS;
                } else {
                    return BAR_OVERLAP_RESULTS_NOT_SEEN_FROM_LONG_PRESS;
                }
            } else {
                if (wasPanelSeen) {
                    return NO_BAR_OVERLAP_RESULTS_SEEN_FROM_LONG_PRESS;
                } else {
                    return NO_BAR_OVERLAP_RESULTS_NOT_SEEN_FROM_LONG_PRESS;
                }
            }
        }
    }

    /**
     * Logs that the conditions are right to force the translation one-box, and whether it
     * was actually forced or not.
     * @param didForceTranslate Whether the translation onebox was forced.
     */
    public static void logTranslateOnebox(boolean didForceTranslate) {
        int code = didForceTranslate ? DID_FORCE_TRANSLATE : WOULD_FORCE_TRANSLATE;
        RecordHistogram.recordEnumeratedHistogram(
                "Search.ContextualSearchShouldTranslate", code, FORCE_TRANSLATE_BOUNDARY);
    }

    /**
     * Logs that whether or not the conditions are met to perform a translation.
     * @param isConditionMet Whether the translation conditions were met.
     */
    public static void logTranslateCondition(boolean isConditionMet) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchTranslateCondition", isConditionMet);
    }

    /**
     * Logs whether Contextual Cards data was shown. Should be logged on tap if Contextual
     * Cards integration is enabled.
     * @param shown Whether Contextual Cards data was shown in the Bar.
     */
    public static void logContextualCardsDataShown(boolean shown) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchContextualCardsIntegration.DataShown", shown);
    }

    /**
     * Logs whether results were seen when Contextual Cards data was shown.
     * @param wasSeen Whether the search results were seen.
     */
    public static void logContextualCardsResultsSeen(boolean wasSeen) {
        RecordHistogram.recordEnumeratedHistogram(
                "Search.ContextualSearchContextualCardsIntegration.ResultsSeen",
                wasSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
    }

    /**
     * Logs whether a quick action intent resolved to zero, one, or many apps.
     * @param quickActionCategory The {@link QuickActionCategory} for the quick action.
     * @param numMatchingAppsApps The number of apps that the resolved intent matched.
     */
    public static void logQuickActionIntentResolution(int quickActionCategory,
            int numMatchingAppsApps) {
        int code = numMatchingAppsApps == 0 ? QUICK_ACTION_RESOLVE_FAILED
                : numMatchingAppsApps == 1 ? QUICK_ACTION_RESOLVE_SINGLE
                        : QUICK_ACTION_RESOLVE_MULTIPLE;
        RecordHistogram.recordEnumeratedHistogram(
                "Search.ContextualSearchQuickActions.IntentResolution."
                        + getLabelForQuickActionCategory(quickActionCategory),
                code, QUICK_ACTION_RESOLVE_BOUNDARY);
    }

    /**
     * Logs whether a quick action was shown, and the quick aciton category if a quick action was
     * shown. Should be logged on tap if Contextual Search single actions are enabled.
     * @param quickActionShown Whether a quick action was shown.
     * @param quickActionCategory The {@link QuickActionCategory} for the quick action.
     */
    public static void logQuickActionShown(boolean quickActionShown, int quickActionCategory) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchQuickActions.Shown", quickActionShown);
        if (quickActionShown) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Search.ContextualSearchQuickActions.Category",
                    quickActionCategory, QuickActionCategory.BOUNDARY);
        }
    }

    /**
     * Logs whether results were seen when a quick action was present.
     * @param wasSeen Whether the search results were seen.
     * @param quickActionCategory The {@link QuickActionCategory} for the quick action.
     */
    public static void logQuickActionResultsSeen(boolean wasSeen, int quickActionCategory) {
        RecordHistogram.recordEnumeratedHistogram(
                "Search.ContextualSearchQuickActions.ResultsSeen."
                        + getLabelForQuickActionCategory(quickActionCategory),
                wasSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
    }

    /**
     * Logs whether a quick action was clicked.
     * @param wasClicked Whether the quick action was clicked
     * @param quickActionCategory The {@link QuickActionCategory} for the quick action.
     */
    public static void logQuickActionClicked(boolean wasClicked, int quickActionCategory) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearchQuickActions.Clicked."
                        + getLabelForQuickActionCategory(quickActionCategory),
                 wasClicked);
    }

    /**
     * Logs results-seen when we have a useful Ranker prediction inference.
     * @param wasPanelSeen Whether the panel was seen.
     * @param predictionKind An integer reflecting the Ranker prediction, e.g. that this is a good
     *        time to suppress triggering because the likelihood of opening the panel is relatively
     *        low.
     */
    public static void logRankerInference(
            boolean wasPanelSeen, @AssistRankerPrediction int predictionKind) {
        if (predictionKind == AssistRankerPrediction.SHOW) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Search.ContextualSearch.Ranker.NotSuppressed.ResultsSeen",
                    wasPanelSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
        } else if (predictionKind == AssistRankerPrediction.SUPPRESS) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Search.ContextualSearch.Ranker.WouldSuppress.ResultsSeen",
                    wasPanelSeen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_BOUNDARY);
        }
    }

    /**
     * Logs Ranker's prediction of whether or not to suppress.
     * @param predictionKind An integer reflecting the Ranker prediction, e.g. that this is a good
     *        time to suppress triggering because the likelihood of opening the panel is relatively
     *        low.
     */
    public static void logRankerPrediction(@AssistRankerPrediction int predictionKind) {
        // For now we just log whether or not suppression is predicted.
        RecordHistogram.recordBooleanHistogram("Search.ContextualSearch.Ranker.Suppressed",
                predictionKind == AssistRankerPrediction.SUPPRESS);
    }

    /** Logs that Ranker recorded a set of features for training or inference. */
    public static void logRecordedFeaturesToRanker() {
        logRecordedToRanker(false);
    }

    /** Logs that Ranker recorded a set of outcomes for training or inference. */
    public static void logRecordedOutcomesToRanker() {
        logRecordedToRanker(true);
    }

    /**
     * Logs that Ranker recorded some data for training or inference.
     * @param areOutcomes Whether the data are outcomes.
     */
    private static void logRecordedToRanker(boolean areOutcomes) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearch.Ranker.Recorded", areOutcomes);
    }

    /**
     * Logs that features or outcomes are available to record to Ranker.
     * This data can be used to correlate with #logRecordedToRanker to validate that everything that
     * should be recorded is actually being recorded.
     * @param areOutcomes Whether the features available are outcomes.
     */
    static void logRankerFeaturesAvailable(boolean areOutcomes) {
        RecordHistogram.recordBooleanHistogram(
                "Search.ContextualSearch.Ranker.FeaturesAvailable", areOutcomes);
    }

    /**
     * Gets the state-change code for the given parameters by doing a lookup in the given map.
     * @param state The panel state.
     * @param reason The reason the state changed.
     * @param stateChangeCodes The map of state and reason to code.
     * @param defaultCode The code to return if the given values are not found in the map.
     * @return The code to write into an enum histogram, based on the given map.
     */
    private static int getStateChangeCode(PanelState state, StateChangeReason reason,
            Map<StateChangeKey, Integer> stateChangeCodes, int defaultCode) {
        Integer code = stateChangeCodes.get(new StateChangeKey(state, reason));
        if (code != null) {
            return code;
        }
        return defaultCode;
    }

    /**
     * Gets the panel-seen code for the given parameters by doing a lookup in the seen-by-gesture
     * map.
     * @param wasPanelSeen Whether the panel was seen.
     * @param wasTap Whether the gesture that originally caused the panel to show was a Tap.
     * @return The code to write into a panel-seen histogram.
     */
    private static int getPanelSeenByGestureStateCode(boolean wasPanelSeen, boolean wasTap) {
        return SEEN_BY_GESTURE_CODES.get(new Pair<Boolean, Boolean>(wasPanelSeen, wasTap));
    }

    /**
     * Gets the promo-outcome code for the given parameter by doing a lookup in the
     * promo-by-gesture map.
     * @param preferenceValue The code for the current preference value.
     * @param wasTap Whether the gesture that originally caused the panel to show was a Tap.
     * @return The code to write into a promo-outcome histogram.
     */
    private static int getPromoByGestureStateCode(int preferenceValue, boolean wasTap) {
        return PROMO_BY_GESTURE_CODES.get(new Pair<Integer, Boolean>(preferenceValue, wasTap));
    }

    /**
     * @return The code for the Contextual Search preference.
     */
    private static int getPreferenceValue() {
        PrefServiceBridge preferences = PrefServiceBridge.getInstance();
        if (preferences.isContextualSearchUninitialized()) {
            return PREFERENCE_UNINITIALIZED;
        } else if (preferences.isContextualSearchDisabled()) {
            return PREFERENCE_DISABLED;
        }
        return PREFERENCE_ENABLED;
    }

    /**
     * Gets the encode value for quick answers seen.
     * @param didActivate Whether the quick answer was shown.
     * @param didAnswer Whether the caption was a full answer, not just a hint.
     * @param wasSeen Whether the search panel was opened.
     * @return The encoded value.
     */
    private static int getQuickAnswerSeenValue(
            boolean didActivate, boolean didAnswer, boolean wasSeen) {
        if (wasSeen) {
            if (didActivate) {
                if (didAnswer) {
                    return QUICK_ANSWER_ACTIVATED_WAS_AN_ANSWER_SEEN;
                } else {
                    return QUICK_ANSWER_ACTIVATED_NOT_AN_ANSWER_SEEN;
                }
            } else {
                return QUICK_ANSWER_NOT_ACTIVATED_SEEN;
            }
        } else {
            if (didActivate) {
                if (didAnswer) {
                    return QUICK_ANSWER_ACTIVATED_WAS_AN_ANSWER_NOT_SEEN;
                } else {
                    return QUICK_ANSWER_ACTIVATED_NOT_AN_ANSWER_NOT_SEEN;
                }
            } else {
                return QUICK_ANSWER_NOT_ACTIVATED_NOT_SEEN;
            }
        }
    }

    /**
     * Logs to a seen-by-gesture histogram of the given name.
     * @param wasPanelSeen Whether the panel was seen.
     * @param wasTap Whether the gesture that originally caused the panel to show was a Tap.
     * @param histogramName The full name of the histogram to log to.
     */
    private static void logHistogramByGesture(boolean wasPanelSeen, boolean wasTap,
            String histogramName) {
        RecordHistogram.recordEnumeratedHistogram(histogramName,
                getPanelSeenByGestureStateCode(wasPanelSeen, wasTap),
                RESULTS_BY_GESTURE_BOUNDARY);
    }

    private static String getLabelForQuickActionCategory(int quickActionCategory) {
        switch(quickActionCategory) {
            case QuickActionCategory.ADDRESS:
                return "Address";
            case QuickActionCategory.EMAIL:
                return "Email";
            case QuickActionCategory.EVENT:
                return "Event";
            case QuickActionCategory.PHONE:
                return "Phone";
            case QuickActionCategory.WEBSITE:
                return "Website";
            default:
                return "None";
        }
    }
}
