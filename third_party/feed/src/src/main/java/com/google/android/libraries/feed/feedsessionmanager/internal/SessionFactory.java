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

package com.google.android.libraries.feed.feedsessionmanager.internal;

import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.time.TimingUtils;

/**
 * Factory for creating a {@link InitializableSession} instance used in the {@link
 * com.google.android.libraries.feed.feedsessionmanager.FeedSessionManager}.
 */
public class SessionFactory {
  private final Store store;
  private final TaskQueue taskQueue;
  private final TimingUtils timingUtils;
  private final ThreadUtils threadUtils;

  public SessionFactory(
      Store store, TaskQueue taskQueue, TimingUtils timingUtils, ThreadUtils threadUtils) {
    this.store = store;
    this.taskQueue = taskQueue;
    this.timingUtils = timingUtils;
    this.threadUtils = threadUtils;
  }

  public InitializableSession getSession() {
    return new SessionImpl(store, taskQueue, timingUtils, threadUtils);
  }

  public HeadSessionImpl getHeadSession() {
    return new HeadSessionImpl(store, timingUtils);
  }
}
