// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.host.scheduler;

import android.support.annotation.IntDef;

/** Allows host to change behavior based on Feed requests and their status */
public interface SchedulerApi {
  /**
   * Define the request behavior of when a new session is created.
   *
   * <ul>
   *   <li>UNKNOWN - Default invalid value, this should not be returned.
   *   <li>REQUEST_WITH_WAIT - Make a request and then wait for it to complete before showing any
   *       content in the Stream.
   *   <li>REQUEST_WITH_CONTENT - Show the user the existing content and make a request. When the
   *       response completes, append the content below any content seen by the user, removing the
   *       old content below that level.
   *   <li>REQUEST_WITH_TIMEOUT - Make a request and if the response takes longer than the
   *       configured timeout, show existing content. When the response completes, the new content
   *       will be displayed below any content the user has viewed.
   *   <li>NO_REQUEST_WITH_WAIT - Show the current content, if an existing request is being made,
   *       wait for it to complete.
   *   <li>NO_REQUEST_WITH_CONTENT - Show the existing content, don't wait if a request is currently
   *       being made.
   *   <li>NO_REQUEST_WITH_TIMEOUT - Show the current content. If an existing request is being made,
   *       wait for the configured timeout for it to complete. When the response completes, the new
   *       content will be displayed below any content the user has viewed.
   * </ul>
   */
  @IntDef({
    RequestBehavior.UNKNOWN,
    RequestBehavior.REQUEST_WITH_WAIT,
    RequestBehavior.REQUEST_WITH_CONTENT,
    RequestBehavior.REQUEST_WITH_TIMEOUT,
    RequestBehavior.NO_REQUEST_WITH_WAIT,
    RequestBehavior.NO_REQUEST_WITH_CONTENT,
    RequestBehavior.NO_REQUEST_WITH_TIMEOUT
  })
  @interface RequestBehavior {
    int UNKNOWN = 0;
    int REQUEST_WITH_WAIT = 1;
    int REQUEST_WITH_CONTENT = 2;
    int REQUEST_WITH_TIMEOUT = 3;
    int NO_REQUEST_WITH_WAIT = 4;
    int NO_REQUEST_WITH_CONTENT = 5;
    int NO_REQUEST_WITH_TIMEOUT = 6;
  }

  /** Object which contains the current state of the SessionManager. */
  class SessionManagerState {
    /** Does the SessionManager have content? */
    public final boolean hasContent;
    /**
     * Returns the date/time of when the content was added. This will only be set when {@code
     * hasContent == true},
     */
    public final long contentCreationDateTimeMs;
    /** Is there an outstanding request being made? */
    public final boolean hasOutstandingRequest;

    public SessionManagerState(
        boolean hasContent, long contentCreationDateTimeMs, boolean hasOutstandingRequest) {
      this.hasContent = hasContent;
      this.contentCreationDateTimeMs = contentCreationDateTimeMs;
      this.hasOutstandingRequest = hasOutstandingRequest;
    }
  }

  /** Called when a Session is created to determine what behavior we should implement. */
  @RequestBehavior
  int shouldSessionRequestData(SessionManagerState sessionManagerState);

  /** Notify scheduler that new content has been received. */
  void onReceiveNewContent();

  /** Notify scheduler that an error occurred handling a request. */
  void onRequestError(int networkResponseCode);
}
