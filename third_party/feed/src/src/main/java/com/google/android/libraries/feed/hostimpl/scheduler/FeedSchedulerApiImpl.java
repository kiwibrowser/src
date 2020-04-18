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

package com.google.android.libraries.feed.hostimpl.scheduler;

import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi;

/**
 * Concrete impl of {@link SchedulerApi}. This implementation determines when a request should be
 * made based upon how old the content currently is.
 */
public class FeedSchedulerApiImpl implements SchedulerApi {

  private final ThreadUtils threadUtils;

  public FeedSchedulerApiImpl(ThreadUtils threadUtils) {
    this.threadUtils = threadUtils;
  }

  @Override
  @RequestBehavior
  public int shouldSessionRequestData(SessionManagerState sessionManagerState) {
    // TODO: Add this back when we move the SchedulerApi use into the session creation
    // threadUtils.checkMainThread();
    return RequestBehavior.REQUEST_WITH_WAIT;
  }

  @Override
  public void onReceiveNewContent() {
    threadUtils.checkMainThread();
    // Do nothing
  }

  @Override
  public void onRequestError(int networkResponseCode) {
    threadUtils.checkMainThread();
    // Do nothing
  }
}
