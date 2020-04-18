// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_REQUEST_LIMITER_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_REQUEST_LIMITER_H_

#include <stddef.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents_observer.h"

class HostContentSettingsMap;

namespace content {
class WebContents;
}

// DownloadRequestLimiter is responsible for determining whether a download
// should be allowed or not. It is designed to keep pages from downloading
// multiple files without user interaction. DownloadRequestLimiter is invoked
// from ResourceDispatcherHost any time a download begins
// (CanDownloadOnIOThread). The request is processed on the UI thread, and the
// request is notified (back on the IO thread) as to whether the download should
// be allowed or denied.
//
// Invoking CanDownloadOnIOThread notifies the callback and may update the
// download status. The following details the various states:
// . Each NavigationController initially starts out allowing a download
//   (ALLOW_ONE_DOWNLOAD).
// . The first time CanDownloadOnIOThread is invoked the download is allowed and
//   the state changes to PROMPT_BEFORE_DOWNLOAD.
// . If the state is PROMPT_BEFORE_DOWNLOAD and the user clicks the mouse,
//   presses enter, the space bar or navigates to another page the state is
//   reset to ALLOW_ONE_DOWNLOAD.
// . If a download is attempted and the state is PROMPT_BEFORE_DOWNLOAD the user
//   is prompted as to whether the download is allowed or disallowed. The users
//   choice stays until the user navigates to a different host. For example, if
//   the user allowed the download, multiple downloads are allowed without any
//   user intervention until the user navigates to a different host.
//
// The DownloadUiStatus indicates whether omnibox UI should be shown for the
// current download status. We do not show UI if there has not yet been
// a download attempt on the page regardless of the internal download status.
class DownloadRequestLimiter
    : public base::RefCountedThreadSafe<DownloadRequestLimiter> {
 public:
  // Download status for a particular page. See class description for details.
  enum DownloadStatus {
    ALLOW_ONE_DOWNLOAD,
    PROMPT_BEFORE_DOWNLOAD,
    ALLOW_ALL_DOWNLOADS,
    DOWNLOADS_NOT_ALLOWED
  };

  // Download UI state given the current download status for a page.
  enum DownloadUiStatus {
    DOWNLOAD_UI_DEFAULT,
    DOWNLOAD_UI_ALLOWED,
    DOWNLOAD_UI_BLOCKED
  };

  // Max number of downloads before a "Prompt Before Download" Dialog is shown.
  static const size_t kMaxDownloadsAtOnce = 50;

  // The callback from CanDownloadOnIOThread. This is invoked on the io thread.
  // The boolean parameter indicates whether or not the download is allowed.
  typedef base::Callback<void(bool /*allow*/)> Callback;

  // TabDownloadState maintains the download state for a particular tab.
  // TabDownloadState prompts the user with an infobar as necessary.
  // TabDownloadState deletes itself (by invoking
  // DownloadRequestLimiter::Remove) as necessary.
  // TODO(gbillock): just make this class implement PermissionRequest.
  class TabDownloadState : public content_settings::Observer,
                           public content::WebContentsObserver {
   public:
    // Creates a new TabDownloadState. |controller| is the controller the
    // TabDownloadState tracks the state of and is the host for any dialogs that
    // are displayed. |originating_controller| is used to determine the host of
    // the initial download. If |originating_controller| is null, |controller|
    // is used. |originating_controller| is typically null, but differs from
    // |controller| in the case of a constrained popup requesting the download.
    TabDownloadState(DownloadRequestLimiter* host,
                     content::WebContents* web_contents,
                     content::WebContents* originating_web_contents);
    ~TabDownloadState() override;

    // Sets the current limiter state and the underlying automatic downloads
    // content setting. Sends a notification that the content setting has been
    // changed (if it has changed).
    void SetDownloadStatusAndNotify(DownloadStatus status);

    // Status of the download.
    DownloadStatus download_status() const { return status_; }

    // The omnibox UI to be showing (or none if we shouldn't show any).
    DownloadUiStatus download_ui_status() const { return ui_status_; }

    // Number of "ALLOWED" downloads.
    void increment_download_count() {
      download_count_++;
    }
    size_t download_count() const {
      return download_count_;
    }

    bool download_seen() const { return download_seen_; }
    void set_download_seen() { download_seen_ = true; }

    // content::WebContentsObserver overrides.
    void DidStartNavigation(
        content::NavigationHandle* navigation_handle) override;
    void DidFinishNavigation(
        content::NavigationHandle* navigation_handle) override;
    // Invoked when a user gesture occurs (mouse click, mouse scroll, tap, or
    // key down). This may result in invoking Remove on DownloadRequestLimiter.
    void DidGetUserInteraction(const blink::WebInputEvent::Type type) override;
    void WebContentsDestroyed() override;

    // Asks the user if they really want to allow the download.
    // See description above CanDownloadOnIOThread for details on lifetime of
    // callback.
    void PromptUserForDownload(
        const DownloadRequestLimiter::Callback& callback);

    // Invoked from DownloadRequestDialogDelegate. Notifies the delegates and
    // changes the status appropriately. Virtual for testing.
    virtual void Cancel();
    virtual void CancelOnce();
    virtual void Accept();

   protected:
    // Used for testing.
    TabDownloadState();

   private:
    // Are we showing a prompt to the user?  Determined by whether
    // we have an outstanding weak pointer--weak pointers are only
    // given to the info bar delegate or permission bubble request.
    bool is_showing_prompt() const;

    // content_settings::Observer overrides.
    void OnContentSettingChanged(
        const ContentSettingsPattern& primary_pattern,
        const ContentSettingsPattern& secondary_pattern,
        ContentSettingsType content_type,
        std::string resource_identifier) override;

    // Remember to either block or allow automatic downloads from this origin.
    void SetContentSetting(ContentSetting setting);

    // Notifies the callbacks as to whether the download is allowed or not.
    // Returns false if it didn't notify all callbacks.
    bool NotifyCallbacks(bool allow);

    // Set the download limiter state and notify if it has changed. Callers must
    // guarantee that |status| and |setting| correspond to each other.
    void SetDownloadStatusAndNotifyImpl(DownloadStatus status,
                                        ContentSetting setting);

    content::WebContents* web_contents_;

    DownloadRequestLimiter* host_;

    // Host of the first page the download started on. This may be empty.
    std::string initial_page_host_;

    DownloadStatus status_;
    DownloadUiStatus ui_status_;

    size_t download_count_;

    // True if a download has been seen on the current page load.
    bool download_seen_;

    // Callbacks we need to notify. This is only non-empty if we're showing a
    // dialog.
    // See description above CanDownloadOnIOThread for details on lifetime of
    // callbacks.
    std::vector<DownloadRequestLimiter::Callback> callbacks_;

    ScopedObserver<HostContentSettingsMap, content_settings::Observer>
        observer_;

    // Weak pointer factory for generating a weak pointer to pass to the
    // infobar.  User responses to the throttling prompt will be returned
    // through this channel, and it can be revoked if the user prompt result
    // becomes moot.
    base::WeakPtrFactory<DownloadRequestLimiter::TabDownloadState> factory_;

    DISALLOW_COPY_AND_ASSIGN(TabDownloadState);
  };

  DownloadRequestLimiter();

  // Returns the download status for a page. This does not change the state in
  // anyway.
  DownloadStatus GetDownloadStatus(content::WebContents* web_contents);

  // Returns the download UI status for a page for the purposes of showing an
  // omnibox decoration.
  DownloadUiStatus GetDownloadUiStatus(content::WebContents* web_contents);

  // Check if download can proceed and notifies the callback on UI thread.
  void CanDownload(const content::ResourceRequestInfo::WebContentsGetter&
                       web_contents_getter,
                   const GURL& url,
                   const std::string& request_method,
                   const Callback& callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(DownloadTest, DownloadResourceThrottleCancels);
  FRIEND_TEST_ALL_PREFIXES(DownloadTest,
                           DownloadRequestLimiterDisallowsAnchorDownloadTag);
  FRIEND_TEST_ALL_PREFIXES(ContentSettingBubbleControllerTest, Init);
  FRIEND_TEST_ALL_PREFIXES(ContentSettingImageModelBrowserTest,
                           CreateBubbleModel);
  friend class base::RefCountedThreadSafe<DownloadRequestLimiter>;
  friend class ContentSettingBubbleDialogTest;
  friend class DownloadRequestLimiterTest;
  friend class TabDownloadState;

  ~DownloadRequestLimiter();

  // Gets the download state for the specified controller. If the
  // TabDownloadState does not exist and |create| is true, one is created.
  // See TabDownloadState's constructor description for details on the two
  // controllers.
  //
  // The returned TabDownloadState is owned by the DownloadRequestLimiter and
  // deleted when no longer needed (the Remove method is invoked).
  TabDownloadState* GetDownloadState(
      content::WebContents* web_contents,
      content::WebContents* originating_web_contents,
      bool create);

  // Does the work of updating the download status on the UI thread and
  // potentially prompting the user.
  void CanDownloadImpl(content::WebContents* originating_contents,
                       const std::string& request_method,
                       const Callback& callback);

  // Invoked when decision to download has been made.
  void OnCanDownloadDecided(
      const content::ResourceRequestInfo::WebContentsGetter&
          web_contents_getter,
      const std::string& request_method,
      const Callback& orig_callback,
      bool allow);

  // Removes the specified TabDownloadState from the internal map and deletes
  // it. This has the effect of resetting the status for the tab to
  // ALLOW_ONE_DOWNLOAD.
  void Remove(TabDownloadState* state, content::WebContents* contents);

  static HostContentSettingsMap* GetContentSettings(
      content::WebContents* contents);

  // Sets the callback for tests to know the result of OnCanDownloadDecided().
  void SetOnCanDownloadDecidedCallbackForTesting(Callback callback);

  // TODO(bauerb): Change this to use WebContentsUserData.
  // Maps from tab to download state. The download state for a tab only exists
  // if the state is other than ALLOW_ONE_DOWNLOAD. Similarly once the state
  // transitions from anything but ALLOW_ONE_DOWNLOAD back to ALLOW_ONE_DOWNLOAD
  // the TabDownloadState is removed and deleted (by way of Remove).
  typedef std::map<content::WebContents*, TabDownloadState*> StateMap;
  StateMap state_map_;

  Callback on_can_download_decided_callback_;

  // Weak ptr factory used when |CanDownload| asks the delegate asynchronously
  // about the download.
  base::WeakPtrFactory<DownloadRequestLimiter> factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadRequestLimiter);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_REQUEST_LIMITER_H_
