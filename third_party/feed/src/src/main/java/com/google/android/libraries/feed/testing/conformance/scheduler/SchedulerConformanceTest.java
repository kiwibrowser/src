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

package com.google.android.libraries.feed.testing.conformance.scheduler;

import com.google.android.libraries.feed.host.scheduler.SchedulerApi;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi.SessionManagerState;
import org.junit.Test;

public abstract class SchedulerConformanceTest {

  private static final int NOT_FOUND = 404;
  private static final int SERVER_ERROR = 500;

  protected SchedulerApi scheduler;

  @Test
  public void shouldSessionRequestData() {
    // Should not throw error
    scheduler.shouldSessionRequestData(new SessionManagerState(false, 0, false));
  }

  @Test
  public void onReceiveNewContent() {
    // Should not throw error
    scheduler.onReceiveNewContent();
  }

  @Test
  public void onRequestError_notFound() {
    // Should not throw error
    scheduler.onRequestError(NOT_FOUND);
  }

  @Test
  public void onRequestError_serverError() {
    // Should not throw error
    scheduler.onRequestError(SERVER_ERROR);
  }
}
