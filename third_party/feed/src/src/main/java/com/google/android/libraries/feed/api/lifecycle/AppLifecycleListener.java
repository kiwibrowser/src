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

package com.google.android.libraries.feed.api.lifecycle;

/**
 * Interface to allow host applications to communicate changes in state to Feed.
 *
 * <p>Note that these are related to app lifecycle, not UI lifecycle
 */
public interface AppLifecycleListener {
  /** Called after critical loading has completed but before Feed is rendered. */
  void onEnterForeground();

  /** Called when the app is backgrounded, will perform clean up. */
  void onEnterBackground();

  /** Called when user opts out of Feed. Will shutdown service and remove notifications. */
  void onOptOut();

  /** Called when user opts into Feed. Will reenable feed and notifications. */
  void onOptIn();

  /**
   * Called when host wants to clear all data. Will delete content without changing opt-in / opt-out
   * status.
   */
  void onClearAll();
}
