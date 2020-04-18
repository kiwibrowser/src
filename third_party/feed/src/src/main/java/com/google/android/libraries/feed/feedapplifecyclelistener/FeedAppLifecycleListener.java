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

package com.google.android.libraries.feed.feedapplifecyclelistener;

import com.google.android.libraries.feed.api.lifecycle.AppLifecycleListener;
import com.google.android.libraries.feed.common.logging.Logger;

/** Default implementation of {@link AppLifecycleListener} */
public class FeedAppLifecycleListener implements AppLifecycleListener {
  private static final String TAG = "FeedAppLifecycleListener";

  @Override
  public void onEnterForeground() {
    Logger.i(TAG, "onEnterForeground called");
  }

  @Override
  public void onEnterBackground() {
    Logger.i(TAG, "onEnterBackground called");
  }

  @Override
  public void onOptOut() {
    Logger.i(TAG, "onOptOut called");
  }

  @Override
  public void onOptIn() {
    Logger.i(TAG, "onOptIn called");
  }

  @Override
  public void onClearAll() {
    Logger.i(TAG, "onClearAll called");
  }
}
