// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_ASCRIBER_SERVICE_H_
#define CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_ASCRIBER_SERVICE_H_

#include <list>
#include <unordered_set>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
class RenderFrameHost;
}

namespace data_use_measurement {
class ChromeDataUseAscriber;

// UI thread functionality of ChromeDataUseAscriber.
//
// Listens to navigation and frame events on the UI thread and propagates them
// to ChromeDataUseAscriber on the IO thread. This class depends on external
// WebContentsObservers to propagate events to itself because each
// WebContents instance requires its own WebContentsObserver instance.
//
// Created, destroyed, and used only on the UI thread.
class ChromeDataUseAscriberService : public KeyedService {
 public:
  ChromeDataUseAscriberService();
  ~ChromeDataUseAscriberService() override;

  // Called when a render frame host is created. Propagates this information to
  // |ascriber_| on the IO thread. |RenderFrameHost| methods cannot be called
  // on the IO thread, so only routing IDs of |render_frame_host| and its parent
  // are propagated.
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host);

  // Called when a render frame host is deleted. Propagates this information to
  // |ascriber_| on the IO thread. RenderFrameHost methods cannot be called
  // on the IO thread, so only routing IDs of |render_frame_host| and its parent
  // are propagated.
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host);

  // Called when the navigation is ready to be committed in a renderer.
  // Propagates the event to the |ascriber_| on the IO thread. NavigationHandle
  // methods cannot be called on the IO thread, so the pointer is cast to void*.
  void ReadyToCommitNavigation(content::NavigationHandle* navigation_handle);

  // Called every time the WebContents changes visibility. Propagates the event
  // to the |ascriber_| on the IO thread.
  void WasShownOrHidden(content::RenderFrameHost* main_render_frame_host,
                        bool visible);

  // Called when one of the render frames of a WebContents is swapped.
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host);

  // Forwarded from DataUseWebContentsObserver
  void DidFinishNavigation(content::NavigationHandle* navigation_handle);

  // Forwarded from DataUseWebContentsObserver
  void DidFinishLoad(content::RenderFrameHost* main_render_frame_host,
                     const GURL& validated_url);

 private:
  friend class ChromeDataUseAscriberServiceTest;

  void SetDataUseAscriber(ChromeDataUseAscriber* ascriber);

  // |ascriber_| outlives this instance.
  ChromeDataUseAscriber* ascriber_;

  // Tracks whether |ascriber_| was set. This field is required because tests
  // might set |ascriber_| to nullptr.
  bool is_initialized_;

  // Frame events might arrive from the UI thread before |ascriber_| is set. A
  // queue of frame events that arrive before |ascriber_| is set is maintained
  // in this field so that they can be propagated immediately after |ascriber_|
  // is set. The RenderFrameHost pointers in the queues are valid for the
  // duration that they are in the queue.
  std::list<content::RenderFrameHost*> pending_frames_queue_;

  // WebContents visibility change events might arrive from the UI thread before
  // |ascriber_| is set. Sucn pending main render frame visible events are
  // maintained in this set and propagated immediately after |ascriber_| is set.
  std::unordered_set<content::RenderFrameHost*> pending_visible_main_frames_;

  DISALLOW_COPY_AND_ASSIGN(ChromeDataUseAscriberService);
};

}  // namespace data_use_measurement

#endif  // CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_ASCRIBER_SERVICE_H_
