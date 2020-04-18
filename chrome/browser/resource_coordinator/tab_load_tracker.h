// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LOAD_TRACKER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LOAD_TRACKER_H_

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/strings/string16.h"

namespace content {
class WebContents;
}  // namespace content

namespace resource_coordinator {

class ResourceCoordinatorTabHelper;
class TabManagerResourceCoordinatorSignalObserverHelper;

// This class has the sole purpose of tracking the state of all tab-related
// WebContents, and whether or not they are in an unloaded, currently loading,
// or loaded state.
//
// This class must be bound to a given Sequence and all access to it must
// occur on that Sequence. In practice, this is intended to be on the UI
// thread as the notifications of interest occur natively on that thread. All
// calculations are very short and quick, so it is suitable for use on that
// thread.
//
// This class is intended to be created in early startup and persists as a
// singleton in the browser process. It is deliberately leaked at shutdown.
//
// This class isn't directly an observer of anything. An external source must
// invoke the callbacks in the protected section of the class. In the case of
// the TabManager this is done by a combination of the
// ResourceCoordinatorTabHelper and the
// TabManagerResourceCoordinatorSignalObserver.
class TabLoadTracker {
 public:
  // An observer class. This allows external classes to be notified of loading
  // state changes.
  class Observer;

  // Indicates the loading state of a WebContents. These values are written to
  // logs and histograms. New enum values can be added, but existing enums must
  // never be renumbered or deleted and reused.
  enum LoadingState {
    // An initially constructed WebContents with no loaded content is UNLOADED.
    // A WebContents that started loading but that errored out before receiving
    // sufficient content to render is also considered UNLOADED.
    // Can only transition from here to LOADING.
    UNLOADED = 0,
    // A WebContents with an ongoing main-frame navigation (to a new document)
    // is in a loading state. More precisely, it is considered loading once
    // network data has started to be transmitted, and not simply when the
    // navigation has started. This considers throttled navigations as not yet
    // loading, and will only transition to loading once the throttle has been
    // removed.
    // Can transition from here to UNLOADED or LOADED.
    LOADING = 1,
    // A WebContents with a committed navigation whose
    // DidStopLoading/PageAlmostIdle event (depending on mode) or DidFailLoad
    // event has fired is no longer considered to be LOADING. If any content has
    // been rendered prior to the failure the document is considered LOADED,
    // otherwise it is considered UNLOADED.
    // Can transition from here to LOADING.
    LOADED = 2,

    // This must be last.
    LOADING_STATE_MAX
  };

  ~TabLoadTracker();

  // Returns the singleton TabLoadTracker instance.
  static TabLoadTracker* Get();

  // Allows querying the state of a tab. The provided |web_contents| must be
  // actively tracked.
  LoadingState GetLoadingState(content::WebContents* web_contents) const;

  // Returns the total number of tabs that are being tracked by this class.
  size_t GetTabCount() const;

  // Returns the number of tabs in each state.
  size_t GetTabCount(LoadingState loading_state) const;
  size_t GetUnloadedTabCount() const;
  size_t GetLoadingTabCount() const;
  size_t GetLoadedTabCount() const;

  // Adds/removes an observer. It is up to the observer to ensure their lifetime
  // exceeds that of the TabLoadTracker, as is removed prior to its destruction.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Exposed so that state transitions can be simulated in tests.
  void TransitionStateForTesting(content::WebContents* web_contents,
                                 LoadingState loading_state);

 protected:
  // This allows the singleton constructor access to the protected constructor.
  friend class base::NoDestructor<TabLoadTracker>;

  // These declarations allows the various bits of TabManager plumbing to
  // forward notifications to the TabLoadTracker.
  friend class resource_coordinator::ResourceCoordinatorTabHelper;
  friend class ::resource_coordinator::
      TabManagerResourceCoordinatorSignalObserverHelper;

  // This class is a singleton so the constructor is protected.
  TabLoadTracker();

  // Initiates tracking of a WebContents. This is fully able to determine the
  // initial state of the WebContents, even if it was created long ago
  // (is LOADING or LOADED) and only just attached to the tracker. See the
  // implementation of DetermineLoadingState for details.
  void StartTracking(content::WebContents* web_contents);

  // Stops tracking a |web_contents|.
  void StopTracking(content::WebContents* web_contents);

  // These are analogs of WebContentsObserver functions. This class is not
  // actually an observer, but the relevant events are forwarded to it from
  // the TabManager.
  void DidStartLoading(content::WebContents* web_contents);
  void DidReceiveResponse(content::WebContents* web_contents);
  void DidStopLoading(content::WebContents* web_contents);
  void DidFailLoad(content::WebContents* web_contents);

  // This is an analog of a PageSignalObserver function. This class is not
  // actually a PageSignalObserver, but these notifications are forwarded to it
  // from the TabManager.
  void OnPageAlmostIdle(content::WebContents* web_contents);

 private:
  // For unittesting.
  friend class TestTabLoadTracker;

  // Some metadata used to track the current state of the WebContents.
  struct WebContentsData {
    LoadingState loading_state = UNLOADED;
    bool did_start_loading_seen = false;
  };

  using TabMap = base::flat_map<content::WebContents*, WebContentsData>;

  // Helper function for determining the current state of a |web_contents|.
  LoadingState DetermineLoadingState(content::WebContents* web_contents);

  // Helper function for marking a load as finished, if possible. If the tab
  // isn't currently marked as loading then this does nothing.
  void MaybeTransitionToLoaded(content::WebContents* web_contents);

  // Transitions a web contents to the given state. This updates the various
  // |state_counts_| and |tabs_| data. Setting |validate_transition| to false
  // means that valid state machine transitions aren't enforced via checks; this
  // is only used by state transitions forced via TransitionStateForTesting.
  void TransitionState(TabMap::iterator it,
                       LoadingState loading_state,
                       bool validate_transition);

  // The list of known WebContents and their states.
  TabMap tabs_;

  // The counts of tabs in each state.
  size_t state_counts_[LOADING_STATE_MAX] = {0};

  base::ObserverList<Observer> observers_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabLoadTracker);
};

// A class for observing loading state changes of WebContents under observation
// by a given TabLoadTracker. All of the callbacks will be invoked on the
// sequence to which the TabLoadTracker is bound.
class TabLoadTracker::Observer {
 public:
  using LoadingState = TabLoadTracker::LoadingState;

  Observer();
  virtual ~Observer();

  // Called when a |web_contents| is starting to be tracked.
  virtual void OnStartTracking(content::WebContents* web_contents,
                               LoadingState loading_state) {}

  // Called for every loading state change observed on a |web_contents|.
  virtual void OnLoadingStateChange(content::WebContents* web_contents,
                                    LoadingState loading_state) {}

  // Called when a |web_contents| is no longer being tracked.
  virtual void OnStopTracking(content::WebContents* web_contents,
                              LoadingState loading_state) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Observer);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LOAD_TRACKER_H_
