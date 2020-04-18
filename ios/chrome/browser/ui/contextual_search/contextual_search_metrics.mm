// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/contextual_search/contextual_search_metrics.h"

#include <map>

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ContextualSearch::PanelState;
using ContextualSearch::StateChangeReason;

// TODO(crbug.com/546238): Convert this into a class that is injected into
// CSController so it can be mocked and tested.

#define VLOG_UMA_HISTOGRAM_ENUMERATION(log_level, name, sample) \
  DVLOG(log_level) << (name) << ": " << (sample);

#define VLOG_UMA_HISTOGRAM_TIMES(log_level, name, sample) \
  DVLOG(log_level) << (name) << ": " << (sample).InMilliseconds();

#define LOGGED_UMA_HISTOGRAM_ENUMERATION(name, sample, boundary, log_level) \
  {                                                                         \
    auto evaluated_sample = static_cast<decltype(boundary)>(sample);        \
    VLOG_UMA_HISTOGRAM_ENUMERATION(log_level, name, evaluated_sample);      \
    UMA_HISTOGRAM_ENUMERATION(name, evaluated_sample, boundary);            \
  }

#define LOGGED_UMA_HISTOGRAM_TIMES(name, sample, log_level) \
  VLOG_UMA_HISTOGRAM_TIMES(log_level, name, sample);        \
  UMA_HISTOGRAM_TIMES(name, sample);

namespace {
// Constants used to log UMA "enum" histograms about the Contextual Search's
// preference state.
const char* const kUMAContextualSearchPreferenceStateHistogram =
    "Search.ContextualSearchPreferenceState";
const char* const kUMAContextualSearchPreferenceStateChangeHistogram =
    "Search.ContextualSearchPreferenceStateChange";
const char* const kUMAContextualSearchFirstRunFlowOutcomeHistogram =
    "Search.ContextualSearchFirstRunFlowOutcome";
enum ContextualSearchPrefState {
  PREFERENCE_UNINITIALIZED = 0,
  PREFERENCE_ENABLED,
  PREFERENCE_DISABLED,
  PREFERENCE_HISTOGRAM_COUNT,
};

// Search duration histograms.
const char* const kUMAContextualSearchDurationSeenHistogram =
    "Search.ContextualSearchDurationSeen";
const char* const kUMAContextualSearchDurationUnseenChainedHistogram =
    "Search.ContextualSearchDurationUnseenChained";
const char* const kUMAContextualSearchDurationUnseenHistogram =
    "Search.ContextualSearchDurationUnseen";
const char* const kUMAContextualSearchTimeToSearchHistogram =
    "Search.ContextualSearchTimeToSearch";

// Constants used to log UMA "enum" histograms about whether search results were
// seen.
const char* const kUMAContextualSearchFirstRunPanelSeenHistogram =
    "Search.ContextualSearchFirstRunPanelSeen";
const char* const kUMAContextualSearchSearchResultsSeenHistogram =
    "Search.ContextualSearchResultsSeen";
enum ContextualSearchResultsSeen {
  RESULTS_SEEN = 0,
  RESULTS_NOT_SEEN,
  RESULTS_SEEN_COUNT,
};

const char* const kUMAContextualSearchSearchResultsSeenByGestureHistogram =
    "Search.ContextualSearchResultsSeenByGesture";
enum ContextualSearchResultsSeenByGesture {
  RESULTS_FROM_TAP_SEEN = 0,
  RESULTS_FROM_TAP_NOT_SEEN,
  RESULTS_FROM_SELECT_SEEN,
  RESULTS_FROM_SELECT_NOT_SEEN,
  RESULTS_SEEN_BY_GESTURE_COUNT,
};

// Constants used to log UMA "enum" histograms about whether the selection is
// valid.
const char* const kUMAContextualSearchSelectionValidHistogram =
    "Search.ContextualSearchSelectionValid";
enum ContextualSearchSelectionValidity {
  SELECTION_VALID = 0,
  SELECTION_INVALID,
  SELECTION_VALIDITY_COUNT,
};

// Constants used to log UMA "enum" histograms about the panel's state
// transitions.
// Entry code: first entry into CLOSED.
const char* const kUMAContextualSearchEnterClosedHistogram =
    "Search.ContextualSearchEnterClosed";
enum ContextualSearchClosedReason {
  ENTER_CLOSED_FROM_OTHER = 0,
  ENTER_CLOSED_FROM_PEEKED_BACK_PRESS,
  ENTER_CLOSED_FROM_PEEKED_BASE_PAGE_SCROLL,
  ENTER_CLOSED_FROM_PEEKED_TEXT_SELECT_TAP,
  ENTER_CLOSED_FROM_EXPANDED_BACK_PRESS,
  ENTER_CLOSED_FROM_EXPANDED_BASE_PAGE_TAP,
  ENTER_CLOSED_FROM_EXPANDED_FLING,
  ENTER_CLOSED_FROM_MAXIMIZED_BACK_PRESS,
  ENTER_CLOSED_FROM_MAXIMIZED_FLING,
  ENTER_CLOSED_FROM_MAXIMIZED_TAB_PROMOTION,
  ENTER_CLOSED_FROM_MAXIMIZED_SERP_NAVIGATION,
  ENTER_CLOSED_FROM_COUNT,
};

// Entry code: first entry into PEEKED.
const char* const kUMAContextualSearchEnterPeekedHistogram =
    "Search.ContextualSearchEnterPeeked";
enum ContextualSearchPeekedReason {
  ENTER_PEEKED_FROM_OTHER = 0,
  ENTER_PEEKED_FROM_CLOSED_TEXT_SELECT_TAP,
  ENTER_PEEKED_FROM_CLOSED_TEXT_SELECT_LONG_PRESS,
  ENTER_PEEKED_FROM_PEEKED_TEXT_SELECT_TAP,
  ENTER_PEEKED_FROM_PEEKED_TEXT_SELECT_LONG_PRESS,
  ENTER_PEEKED_FROM_EXPANDED_SEARCH_BAR_TAP,
  ENTER_PEEKED_FROM_EXPANDED_SWIPE,
  ENTER_PEEKED_FROM_EXPANDED_FLING,
  ENTER_PEEKED_FROM_MAXIMIZED_SWIPE,
  ENTER_PEEKED_FROM_MAXIMIZED_FLING,
  ENTER_PEEKED_FROM_COUNT,
};

// Entry code: first entry into EXPANDED.
const char* const kUMAContextualSearchEnterExpandedHistogram =
    "Search.ContextualSearchEnterExpanded";
enum ContextualSearchExpandedReason {
  ENTER_EXPANDED_FROM_OTHER = 0,
  ENTER_EXPANDED_FROM_PEEKED_SEARCH_BAR_TAP,
  ENTER_EXPANDED_FROM_PEEKED_SWIPE,
  ENTER_EXPANDED_FROM_PEEKED_FLING,
  ENTER_EXPANDED_FROM_MAXIMIZED_SWIPE,
  ENTER_EXPANDED_FROM_MAXIMIZED_FLING,
  ENTER_EXPANDED_FROM_COUNT,
};

// Entry code: first entry into MAXIMIZED.
const char* const kUMAContextualSearchEnterMaximizedHistogram =
    "Search.ContextualSearchEnterMaximized";
enum ContextualSearchMaximizedReason {
  ENTER_MAXIMIZED_FROM_OTHER = 0,
  ENTER_MAXIMIZED_FROM_PEEKED_SWIPE,
  ENTER_MAXIMIZED_FROM_PEEKED_FLING,
  ENTER_MAXIMIZED_FROM_EXPANDED_SWIPE,
  ENTER_MAXIMIZED_FROM_EXPANDED_FLING,
  ENTER_MAXIMIZED_FROM_EXPANDED_SERP_NAVIGATION,
  ENTER_MAXIMIZED_FROM_COUNT,
};

// Exit code: first exit from CLOSED (or UNDEFINED).
const char* const kUMAContextualSearchExitClosedHistogram =
    "Search.ContextualSearchExitClosed";
enum ContextualSearchClosedExit {
  EXIT_CLOSED_TO_OTHER = 0,
  EXIT_CLOSED_TO_PEEKED_TEXT_SELECT_TAP,
  EXIT_CLOSED_TO_PEEKED_TEXT_SELECT_LONG_PRESS,
  EXIT_CLOSED_TO_COUNT,
};

// Exit code: first exit from PEEKED.
const char* const kUMAContextualSearchExitPeekedHistogram =
    "Search.ContextualSearchExitPeeked";
enum ContextualSearchPeekedExit {
  EXIT_PEEKED_TO_OTHER = 0,
  EXIT_PEEKED_TO_CLOSED_BACK_PRESS,
  EXIT_PEEKED_TO_CLOSED_BASE_PAGE_SCROLL,
  EXIT_PEEKED_TO_CLOSED_TEXT_SELECT_TAP,
  EXIT_PEEKED_TO_PEEKED_TEXT_SELECT_TAP,
  EXIT_PEEKED_TO_PEEKED_TEXT_SELECT_LONG_PRESS,
  EXIT_PEEKED_TO_EXPANDED_SEARCH_BAR_TAP,
  EXIT_PEEKED_TO_EXPANDED_SWIPE,
  EXIT_PEEKED_TO_EXPANDED_FLING,
  EXIT_PEEKED_TO_MAXIMIZED_SWIPE,
  EXIT_PEEKED_TO_MAXIMIZED_FLING,
  EXIT_PEEKED_TO_COUNT,
};

// Exit code: first exit from EXPANDED.
const char* const kUMAContextualSearchExitExpandedHistogram =
    "Search.ContextualSearchExitExpanded";
enum ContextualSearchExpandedExit {
  EXIT_EXPANDED_TO_OTHER = 0,
  EXIT_EXPANDED_TO_CLOSED_BACK_PRESS,
  EXIT_EXPANDED_TO_CLOSED_BASE_PAGE_TAP,
  EXIT_EXPANDED_TO_CLOSED_FLING,
  EXIT_EXPANDED_TO_PEEKED_SEARCH_BAR_TAP,
  EXIT_EXPANDED_TO_PEEKED_SWIPE,
  EXIT_EXPANDED_TO_PEEKED_FLING,
  EXIT_EXPANDED_TO_MAXIMIZED_SWIPE,
  EXIT_EXPANDED_TO_MAXIMIZED_FLING,
  EXIT_EXPANDED_TO_MAXIMIZED_SERP_NAVIGATION,
  EXIT_EXPANDED_TO_COUNT,
};

// Exit code: first exit from MAXIMIZED.
const char* const kUMAContextualSearchExitMaximizedHistogram =
    "Search.ContextualSearchExitMaximized";
enum ContextualSearchMaximizedExit {
  EXIT_MAXIMIZED_TO_OTHER = 0,
  EXIT_MAXIMIZED_TO_CLOSED_BACK_PRESS,
  EXIT_MAXIMIZED_TO_CLOSED_FLING,
  EXIT_MAXIMIZED_TO_CLOSED_TAB_PROMOTION,
  EXIT_MAXIMIZED_TO_CLOSED_SERP_NAVIGATION,
  EXIT_MAXIMIZED_TO_PEEKED_SWIPE,
  EXIT_MAXIMIZED_TO_PEEKED_FLING,
  EXIT_MAXIMIZED_TO_EXPANDED_SWIPE,
  EXIT_MAXIMIZED_TO_EXPANDED_FLING,
  EXIT_MAXIMIZED_TO_COUNT,
};

// Panel states that match UMA enum names.
enum PanelState { UNDEFINED, CLOSED, PEEKED, EXPANDED, MAXIMIZED };

// Utility method to convert enter transition (|from_state|, CLOSED, |reason|)
// to UMA histogram value.
int EnterClosedStateChanges(PanelState from_state,
                            ContextualSearch::StateChangeReason reason,
                            int default_code) {
  switch (from_state) {
    case PEEKED:
      switch (reason) {
        case ContextualSearch::BACK_PRESS:
          return ENTER_CLOSED_FROM_PEEKED_BACK_PRESS;
        case ContextualSearch::BASE_PAGE_SCROLL:
          return ENTER_CLOSED_FROM_PEEKED_BASE_PAGE_SCROLL;
        case ContextualSearch::TEXT_SELECT_TAP:
          return ENTER_CLOSED_FROM_PEEKED_TEXT_SELECT_TAP;
        default:
          break;
      }
      break;
    case EXPANDED:
      switch (reason) {
        case ContextualSearch::BACK_PRESS:
          return ENTER_CLOSED_FROM_EXPANDED_BACK_PRESS;
        case ContextualSearch::BASE_PAGE_TAP:
          return ENTER_CLOSED_FROM_EXPANDED_BASE_PAGE_TAP;
        case ContextualSearch::FLING:
          return ENTER_CLOSED_FROM_EXPANDED_FLING;
        default:
          break;
      }
      break;
    case MAXIMIZED:
      switch (reason) {
        case ContextualSearch::BACK_PRESS:
          return ENTER_CLOSED_FROM_MAXIMIZED_BACK_PRESS;
        case ContextualSearch::FLING:
          return ENTER_CLOSED_FROM_MAXIMIZED_FLING;
        case ContextualSearch::TAB_PROMOTION:
          return ENTER_CLOSED_FROM_MAXIMIZED_TAB_PROMOTION;
        case ContextualSearch::SERP_NAVIGATION:
          return ENTER_CLOSED_FROM_MAXIMIZED_SERP_NAVIGATION;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return default_code;
}

// Utility method to convert enter transition (|from_state|, PEEKED, |reason|)
// to UMA histogram value.
int EnterPeekedStateChanges(PanelState from_state,
                            ContextualSearch::StateChangeReason reason,
                            int default_code) {
  switch (from_state) {
    case CLOSED:
      switch (reason) {
        case ContextualSearch::TEXT_SELECT_TAP:
          return ENTER_PEEKED_FROM_CLOSED_TEXT_SELECT_TAP;
        case ContextualSearch::TEXT_SELECT_LONG_PRESS:
          return ENTER_PEEKED_FROM_CLOSED_TEXT_SELECT_LONG_PRESS;
        default:
          break;
      }
      break;
    case PEEKED:
      switch (reason) {
        case ContextualSearch::TEXT_SELECT_TAP:
          return ENTER_PEEKED_FROM_PEEKED_TEXT_SELECT_TAP;
        case ContextualSearch::TEXT_SELECT_LONG_PRESS:
          return ENTER_PEEKED_FROM_PEEKED_TEXT_SELECT_LONG_PRESS;
        default:
          break;
      }
      break;
    case EXPANDED:
      switch (reason) {
        case ContextualSearch::SEARCH_BAR_TAP:
          return ENTER_PEEKED_FROM_EXPANDED_SEARCH_BAR_TAP;
        case ContextualSearch::SWIPE:
          return ENTER_PEEKED_FROM_EXPANDED_SWIPE;
        case ContextualSearch::FLING:
          return ENTER_PEEKED_FROM_EXPANDED_FLING;
        default:
          break;
      }
      break;
    case MAXIMIZED:
      switch (reason) {
        case ContextualSearch::SWIPE:
          return ENTER_PEEKED_FROM_MAXIMIZED_SWIPE;
        case ContextualSearch::FLING:
          return ENTER_PEEKED_FROM_MAXIMIZED_FLING;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return default_code;
}

// Utility method to convert enter transition (|from_state|, EXPANDED, |reason|)
// to UMA histogram value.
int EnterExpandedStateChanges(PanelState from_state,
                              ContextualSearch::StateChangeReason reason,
                              int default_code) {
  switch (from_state) {
    case PEEKED:
      switch (reason) {
        case ContextualSearch::SEARCH_BAR_TAP:
          return ENTER_EXPANDED_FROM_PEEKED_SEARCH_BAR_TAP;
        case ContextualSearch::SWIPE:
          return ENTER_EXPANDED_FROM_PEEKED_SWIPE;
        case ContextualSearch::FLING:
          return ENTER_EXPANDED_FROM_PEEKED_FLING;
        default:
          break;
      }
      break;
    case MAXIMIZED:
      switch (reason) {
        case ContextualSearch::SWIPE:
          return ENTER_EXPANDED_FROM_MAXIMIZED_SWIPE;
        case ContextualSearch::FLING:
          return ENTER_EXPANDED_FROM_MAXIMIZED_FLING;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return default_code;
}

// Utility method to convert enter transition
// (|from_state|, MAXIMIZED, |reason|) to UMA histogram value.
int EnterMaximizedStateChanges(PanelState from_state,
                               ContextualSearch::StateChangeReason reason,
                               int default_code) {
  switch (from_state) {
    case PEEKED:
      switch (reason) {
        case ContextualSearch::SWIPE:
          return ENTER_MAXIMIZED_FROM_PEEKED_SWIPE;
        case ContextualSearch::FLING:
          return ENTER_MAXIMIZED_FROM_PEEKED_FLING;
        default:
          break;
      }
      break;
    case EXPANDED:
      switch (reason) {
        case ContextualSearch::SWIPE:
          return ENTER_MAXIMIZED_FROM_EXPANDED_SWIPE;
        case ContextualSearch::FLING:
          return ENTER_MAXIMIZED_FROM_EXPANDED_FLING;
        case ContextualSearch::SERP_NAVIGATION:
          return ENTER_MAXIMIZED_FROM_EXPANDED_SERP_NAVIGATION;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return default_code;
}

// Utility method to convert enter exit (CLOSED, |to_state|, |reason|)
// to UMA histogram value.
int ExitClosedStateChanges(PanelState to_state,
                           ContextualSearch::StateChangeReason reason,
                           int default_code) {
  switch (to_state) {
    case PEEKED:
      switch (reason) {
        case ContextualSearch::TEXT_SELECT_TAP:
          return EXIT_CLOSED_TO_PEEKED_TEXT_SELECT_TAP;
        case ContextualSearch::TEXT_SELECT_LONG_PRESS:
          return EXIT_CLOSED_TO_PEEKED_TEXT_SELECT_LONG_PRESS;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return default_code;
}

// Utility method to convert enter exit (PEEKED, |to_state|, |reason|)
// to UMA histogram value.
int ExitPeekedStateChanges(PanelState to_state,
                           ContextualSearch::StateChangeReason reason,
                           int default_code) {
  switch (to_state) {
    case CLOSED:
      switch (reason) {
        case ContextualSearch::BACK_PRESS:
          return EXIT_PEEKED_TO_CLOSED_BACK_PRESS;
        case ContextualSearch::BASE_PAGE_SCROLL:
          return EXIT_PEEKED_TO_CLOSED_BASE_PAGE_SCROLL;
        case ContextualSearch::TEXT_SELECT_TAP:
          return EXIT_PEEKED_TO_CLOSED_TEXT_SELECT_TAP;
        default:
          break;
      }
      break;
    case PEEKED:
      switch (reason) {
        case ContextualSearch::TEXT_SELECT_TAP:
          return EXIT_PEEKED_TO_PEEKED_TEXT_SELECT_TAP;
        case ContextualSearch::TEXT_SELECT_LONG_PRESS:
          return EXIT_PEEKED_TO_PEEKED_TEXT_SELECT_LONG_PRESS;
        default:
          break;
      }
      break;
    case EXPANDED:
      switch (reason) {
        case ContextualSearch::SEARCH_BAR_TAP:
          return EXIT_PEEKED_TO_EXPANDED_SEARCH_BAR_TAP;
        case ContextualSearch::SWIPE:
          return EXIT_PEEKED_TO_EXPANDED_SWIPE;
        case ContextualSearch::FLING:
          return EXIT_PEEKED_TO_EXPANDED_FLING;
        default:
          break;
      }
      break;
    case MAXIMIZED:
      switch (reason) {
        case ContextualSearch::SWIPE:
          return EXIT_PEEKED_TO_MAXIMIZED_SWIPE;
        case ContextualSearch::FLING:
          return EXIT_PEEKED_TO_MAXIMIZED_FLING;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return default_code;
}

// Utility method to convert enter exit (EXPANDED, |to_state|, |reason|)
// to UMA histogram value.
int ExitExpandedStateChanges(PanelState to_state,
                             ContextualSearch::StateChangeReason reason,
                             int default_code) {
  switch (to_state) {
    case CLOSED:
      switch (reason) {
        case ContextualSearch::BACK_PRESS:
          return EXIT_EXPANDED_TO_CLOSED_BACK_PRESS;
        case ContextualSearch::BASE_PAGE_TAP:
          return EXIT_EXPANDED_TO_CLOSED_BASE_PAGE_TAP;
        case ContextualSearch::FLING:
          return EXIT_EXPANDED_TO_CLOSED_FLING;
        default:
          break;
      }
      break;
    case PEEKED:
      switch (reason) {
        case ContextualSearch::SEARCH_BAR_TAP:
          return EXIT_EXPANDED_TO_PEEKED_SEARCH_BAR_TAP;
        case ContextualSearch::SWIPE:
          return EXIT_EXPANDED_TO_PEEKED_SWIPE;
        case ContextualSearch::FLING:
          return EXIT_EXPANDED_TO_PEEKED_FLING;
        default:
          break;
      }
      break;
    case MAXIMIZED:
      switch (reason) {
        case ContextualSearch::SWIPE:
          return EXIT_EXPANDED_TO_MAXIMIZED_SWIPE;
        case ContextualSearch::FLING:
          return EXIT_EXPANDED_TO_MAXIMIZED_FLING;
        case ContextualSearch::SERP_NAVIGATION:
          return EXIT_EXPANDED_TO_MAXIMIZED_SERP_NAVIGATION;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return default_code;
}

// Utility method to convert enter exit (MAXIMIZED, |to_state|, |reason|)
// to UMA histogram value.
int ExitMaximizedStateChanges(PanelState to_state,
                              ContextualSearch::StateChangeReason reason,
                              int default_code) {
  switch (to_state) {
    case CLOSED:
      switch (reason) {
        case ContextualSearch::BACK_PRESS:
          return EXIT_MAXIMIZED_TO_CLOSED_BACK_PRESS;
        case ContextualSearch::FLING:
          return EXIT_MAXIMIZED_TO_CLOSED_FLING;
        case ContextualSearch::TAB_PROMOTION:
          return EXIT_MAXIMIZED_TO_CLOSED_TAB_PROMOTION;
        case ContextualSearch::SERP_NAVIGATION:
          return EXIT_MAXIMIZED_TO_CLOSED_SERP_NAVIGATION;
        default:
          break;
      }
      break;
    case PEEKED:
      switch (reason) {
        case ContextualSearch::SWIPE:
          return EXIT_MAXIMIZED_TO_PEEKED_SWIPE;
        case ContextualSearch::FLING:
          return EXIT_MAXIMIZED_TO_PEEKED_FLING;
        default:
          break;
      }
      break;
    case EXPANDED:
      switch (reason) {
        case ContextualSearch::SWIPE:
          return EXIT_MAXIMIZED_TO_EXPANDED_SWIPE;
        case ContextualSearch::FLING:
          return EXIT_MAXIMIZED_TO_EXPANDED_FLING;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return default_code;
}

// Utility method that extracts a state change UMA enum value
int getStateChangeCode(PanelState from_state,
                       ContextualSearch::StateChangeReason reason,
                       PanelState to_state,
                       bool entry,
                       int default_code) {
  if (entry) {
    DCHECK_NE(from_state, UNDEFINED);
    switch (to_state) {
      case CLOSED:
        return EnterClosedStateChanges(from_state, reason, default_code);
      case PEEKED:
        return EnterPeekedStateChanges(from_state, reason, default_code);
      case EXPANDED:
        return EnterExpandedStateChanges(from_state, reason, default_code);
      case MAXIMIZED:
        return EnterMaximizedStateChanges(from_state, reason, default_code);
      default:
        return default_code;
    }
  } else {
    switch (from_state) {
      case UNDEFINED:
      case CLOSED:
        return ExitClosedStateChanges(to_state, reason, default_code);
      case PEEKED:
        return ExitPeekedStateChanges(to_state, reason, default_code);
      case EXPANDED:
        return ExitExpandedStateChanges(to_state, reason, default_code);
      case MAXIMIZED:
        return ExitMaximizedStateChanges(to_state, reason, default_code);
      default:
        return default_code;
    }
  }
}

// Utility method that maps ContextualSearch::PanelStates to PanelStates.
PanelState panelState(ContextualSearch::PanelState state) {
  switch (state) {
    case ContextualSearch::UNDEFINED:
      return UNDEFINED;
    case ContextualSearch::DISMISSED:
      return CLOSED;
    case ContextualSearch::PEEKING:
      return PEEKED;
    case ContextualSearch::PREVIEWING:
      return EXPANDED;
    case ContextualSearch::COVERING:
      return MAXIMIZED;
    default:
      NOTREACHED() << "Exciting new ContextualSearchPanel state found!";
      return UNDEFINED;
  }
}

// Utility method that mapse xternal pref state enum value into internal pref
// state histogram value (note that we want to keep these decoupled).
ContextualSearchPrefState getPrefState(
    TouchToSearch::TouchToSearchPreferenceState prefState) {
  switch (prefState) {
    case TouchToSearch::DISABLED:
      return PREFERENCE_DISABLED;
    case TouchToSearch::ENABLED:
      return PREFERENCE_ENABLED;
    case TouchToSearch::UNDECIDED:
      return PREFERENCE_UNINITIALIZED;
    default:
      NOTREACHED();
  }
}

// Utility method for recording the results-seen metric common to multiple
// metrics calls.
void RecordResultsSeen(bool seen) {
  LOGGED_UMA_HISTOGRAM_ENUMERATION(
      kUMAContextualSearchSearchResultsSeenHistogram,
      seen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_COUNT, 1);
}

}  // namespace

namespace ContextualSearch {

void RecordPreferenceState(TouchToSearch::TouchToSearchPreferenceState state) {
  LOGGED_UMA_HISTOGRAM_ENUMERATION(kUMAContextualSearchPreferenceStateHistogram,
                                   getPrefState(state),
                                   PREFERENCE_HISTOGRAM_COUNT, 1);
}

void RecordPreferenceChanged(bool enabled) {
  LOGGED_UMA_HISTOGRAM_ENUMERATION(
      kUMAContextualSearchPreferenceStateChangeHistogram,
      enabled ? PREFERENCE_ENABLED : PREFERENCE_DISABLED,
      PREFERENCE_HISTOGRAM_COUNT, 1);
}

void RecordFirstRunFlowOutcome(
    TouchToSearch::TouchToSearchPreferenceState state) {
  LOGGED_UMA_HISTOGRAM_ENUMERATION(
      kUMAContextualSearchFirstRunFlowOutcomeHistogram, getPrefState(state),
      PREFERENCE_HISTOGRAM_COUNT, 1);
}

void RecordDuration(bool resultsSeen, bool chained, base::TimeDelta duration) {
  if (resultsSeen) {
    LOGGED_UMA_HISTOGRAM_TIMES(kUMAContextualSearchDurationSeenHistogram,
                               duration, 1);
  } else if (chained) {
    LOGGED_UMA_HISTOGRAM_TIMES(
        kUMAContextualSearchDurationUnseenChainedHistogram, duration, 1);
  } else {
    LOGGED_UMA_HISTOGRAM_TIMES(kUMAContextualSearchDurationUnseenHistogram,
                               duration, 1);
  }
}

void RecordTimeToSearch(base::TimeDelta duration) {
  LOGGED_UMA_HISTOGRAM_TIMES(kUMAContextualSearchTimeToSearchHistogram,
                             duration, 1);
}

void RecordFirstRunPanelSeen(bool seen) {
  LOGGED_UMA_HISTOGRAM_ENUMERATION(
      kUMAContextualSearchFirstRunPanelSeenHistogram,
      seen ? RESULTS_SEEN : RESULTS_NOT_SEEN, RESULTS_SEEN_COUNT, 1);
}

void RecordTapResultsSeen(bool seen) {
  RecordResultsSeen(seen);
  LOGGED_UMA_HISTOGRAM_ENUMERATION(
      kUMAContextualSearchSearchResultsSeenByGestureHistogram,
      seen ? RESULTS_FROM_TAP_SEEN : RESULTS_FROM_TAP_NOT_SEEN,
      RESULTS_SEEN_BY_GESTURE_COUNT, 1);
}

void RecordSelectionResultsSeen(bool seen) {
  RecordResultsSeen(seen);
  LOGGED_UMA_HISTOGRAM_ENUMERATION(
      kUMAContextualSearchSearchResultsSeenByGestureHistogram,
      seen ? RESULTS_FROM_SELECT_SEEN : RESULTS_FROM_SELECT_NOT_SEEN,
      RESULTS_SEEN_BY_GESTURE_COUNT, 1);
}

void RecordSelectionIsValid(bool valid) {
  LOGGED_UMA_HISTOGRAM_ENUMERATION(kUMAContextualSearchSelectionValidHistogram,
                                   valid ? SELECTION_VALID : SELECTION_INVALID,
                                   SELECTION_VALIDITY_COUNT, 1);
}

void RecordFirstStateEntry(PanelState from_state,
                           PanelState to_state,
                           StateChangeReason reason) {
  switch (panelState(to_state)) {
    case CLOSED:
      LOGGED_UMA_HISTOGRAM_ENUMERATION(
          kUMAContextualSearchEnterClosedHistogram,
          getStateChangeCode(panelState(from_state), reason, CLOSED, true,
                             ENTER_CLOSED_FROM_OTHER),
          ENTER_CLOSED_FROM_COUNT, 1);
      break;
    case PEEKED:
      LOGGED_UMA_HISTOGRAM_ENUMERATION(
          kUMAContextualSearchEnterPeekedHistogram,
          getStateChangeCode(panelState(from_state), reason, PEEKED, true,
                             ENTER_PEEKED_FROM_OTHER),
          ENTER_PEEKED_FROM_COUNT, 1);
      break;
    case EXPANDED:
      LOGGED_UMA_HISTOGRAM_ENUMERATION(
          kUMAContextualSearchEnterExpandedHistogram,
          getStateChangeCode(panelState(from_state), reason, EXPANDED, true,
                             ENTER_EXPANDED_FROM_OTHER),
          ENTER_EXPANDED_FROM_COUNT, 1);
      break;
    case MAXIMIZED:
      LOGGED_UMA_HISTOGRAM_ENUMERATION(
          kUMAContextualSearchEnterMaximizedHistogram,
          getStateChangeCode(panelState(from_state), reason, MAXIMIZED, true,
                             ENTER_MAXIMIZED_FROM_OTHER),
          ENTER_MAXIMIZED_FROM_COUNT, 1);
      break;
    default:
      NOTREACHED() << "RecordFirstStateEntry for unexpected state " << to_state;
      break;
  }
}

void RecordFirstStateExit(PanelState from_state,
                          PanelState to_state,
                          StateChangeReason reason) {
  switch (panelState(from_state)) {
    case CLOSED:
      LOGGED_UMA_HISTOGRAM_ENUMERATION(
          kUMAContextualSearchExitClosedHistogram,
          getStateChangeCode(CLOSED, reason, panelState(to_state), false,
                             EXIT_CLOSED_TO_OTHER),
          EXIT_CLOSED_TO_COUNT, 1);
      break;
    case PEEKED:
      LOGGED_UMA_HISTOGRAM_ENUMERATION(
          kUMAContextualSearchExitPeekedHistogram,
          getStateChangeCode(PEEKED, reason, panelState(to_state), false,
                             EXIT_PEEKED_TO_OTHER),
          EXIT_PEEKED_TO_COUNT, 1);
      break;
    case EXPANDED:
      LOGGED_UMA_HISTOGRAM_ENUMERATION(
          kUMAContextualSearchExitExpandedHistogram,
          getStateChangeCode(EXPANDED, reason, panelState(to_state), false,
                             EXIT_EXPANDED_TO_OTHER),
          EXIT_EXPANDED_TO_COUNT, 1);
      break;
    case MAXIMIZED:
      LOGGED_UMA_HISTOGRAM_ENUMERATION(
          kUMAContextualSearchExitMaximizedHistogram,
          getStateChangeCode(MAXIMIZED, reason, panelState(to_state), false,
                             EXIT_MAXIMIZED_TO_OTHER),
          EXIT_MAXIMIZED_TO_COUNT, 1);
      break;
    default:
      NOTREACHED() << "RecordFirstStateExit for unexpected state " << to_state;
      break;
  }
}

}  // namespace ContextualSearch
