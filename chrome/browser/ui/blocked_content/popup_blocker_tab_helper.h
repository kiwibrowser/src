// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_BLOCKER_TAB_HELPER_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_BLOCKER_TAB_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "chrome/browser/ui/blocked_content/blocked_window_params.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/base/window_open_disposition.h"

struct NavigateParams;

namespace content {
struct OpenURLParams;
}

class GURL;
class SafeBrowsingTriggeredPopupBlocker;

// Per-tab class to manage blocked popups.
class PopupBlockerTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PopupBlockerTabHelper> {
 public:
  // Mapping from popup IDs to blocked popup requests.
  typedef std::map<int32_t, GURL> PopupIdMap;

  // Classifies what caused a popup to be blocked.
  enum class PopupBlockType {
    // Popup blocked due to no user gesture.
    kNoGesture,
    // Popup blocked due to the abusive popup blocker.
    kAbusive,
  };

  // This enum is backed by a histogram. Make sure enums.xml is updated if this
  // is updated.
  enum class Action : int {
    // A popup was initiated and was sent to the popup blocker for
    // consideration.
    kInitiated,

    // A popup was blocked by the popup blocker.
    kBlocked,

    // A previously blocked popup was clicked through. For popups blocked
    // without a user gesture.
    kClickedThroughNoGesture,

    // A previously blocked popup was clicked through. For popups blocked
    // due to the abusive popup blocker.
    kClickedThroughAbusive,

    // Add new elements before this value.
    kLast
  };

  class Observer {
   public:
    virtual void BlockedPopupAdded(int32_t id, const GURL& url) {}

   protected:
    virtual ~Observer() = default;
  };

  ~PopupBlockerTabHelper() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Whether a new window opened with |disposition| would be considered for
  // popup blocking. Note that this includes more dispositions than just
  // NEW_POPUP since the popup blocker targets all new windows and tabs.
  static bool ConsiderForPopupBlocking(WindowOpenDisposition disposition);

  // Returns true if the popup request defined by |params| and the optional
  // |open_url_params| should be blocked. In that case, it is also added to the
  // |blocked_popups_| container.
  //
  // |opener_url| is an optional parameter used to compute how the popup
  // permission will behave. If it is not set the current committed URL will be
  // used instead.
  //
  // If this function returns true, then the contents of |params| is moved to
  // |blocked_popups_|.
  static bool MaybeBlockPopup(
      content::WebContents* web_contents,
      const base::Optional<GURL>& opener_url,
      NavigateParams* params,
      const content::OpenURLParams* open_url_params,
      const blink::mojom::WindowFeatures& window_features);

  // Creates the blocked popup with |popup_id| in given |dispostion|.
  // Note that if |disposition| is WindowOpenDisposition::CURRENT_TAB,
  // blocked popup will be opened as it was specified by renderer.
  void ShowBlockedPopup(int32_t popup_id, WindowOpenDisposition disposition);

  // Returns the number of blocked popups.
  size_t GetBlockedPopupsCount() const;

  PopupIdMap GetBlockedPopupRequests();

  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  struct BlockedRequest;
  friend class content::WebContentsUserData<PopupBlockerTabHelper>;

  explicit PopupBlockerTabHelper(content::WebContents* web_contents);

  void AddBlockedPopup(NavigateParams* params,
                       const blink::mojom::WindowFeatures& window_features,
                       PopupBlockType block_type);

  // Called when the blocked popup notification is shown or hidden.
  void PopupNotificationVisibilityChanged(bool visible);

  static void LogAction(Action action);

  // Note, this container should be sorted based on the position in the popup
  // list, so it is keyed by an id which is continually increased.
  std::map<int32_t, std::unique_ptr<BlockedRequest>> blocked_popups_;

  base::ObserverList<Observer> observers_;

  int32_t next_id_ = 0;

  std::unique_ptr<SafeBrowsingTriggeredPopupBlocker>
      safe_browsing_triggered_popup_blocker_;

  DISALLOW_COPY_AND_ASSIGN(PopupBlockerTabHelper);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_BLOCKER_TAB_HELPER_H_
