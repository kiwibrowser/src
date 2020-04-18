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

package com.google.android.libraries.feed.feedmodelprovider;

import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.host.config.Configuration;

/**
 * Factory for creating instances of {@link ModelProviderFactory} using the {@link
 * FeedModelProvider}.
 */
public class FeedModelProviderFactory implements ModelProviderFactory {
  private final SessionManager sessionManager;
  private final ThreadUtils threadUtils;
  private final TimingUtils timingUtils;
  private final MainThreadRunner mainThreadRunner;
  private final Configuration config;

  public FeedModelProviderFactory(
      SessionManager sessionManager,
      ThreadUtils threadUtils,
      TimingUtils timingUtils,
      MainThreadRunner mainThreadRunner,
      Configuration config) {
    this.sessionManager = sessionManager;
    this.threadUtils = threadUtils;
    this.timingUtils = timingUtils;
    this.mainThreadRunner = mainThreadRunner;
    this.config = config;
  }

  @Override
  /*@Nullable*/
  public ModelProvider create(String sessionToken) {
    FeedModelProvider modelProvider =
        new FeedModelProvider(sessionManager, threadUtils, timingUtils, mainThreadRunner, config);
    boolean success = sessionManager.getExistingSession(sessionToken, modelProvider);
    return (success) ? modelProvider : null;
  }

  @Override
  public ModelProvider createNew() {
    FeedModelProvider modelProvider =
        new FeedModelProvider(sessionManager, threadUtils, timingUtils, mainThreadRunner, config);
    sessionManager.getNewSession(modelProvider);
    return modelProvider;
  }
}
