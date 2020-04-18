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

package com.google.android.libraries.feed.common.testing;

import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.protoextensions.FeedExtensionRegistry;
import com.google.android.libraries.feed.common.time.Clock;
import com.google.android.libraries.feed.common.time.SystemClockImpl;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.feedmodelprovider.FeedModelProviderFactory;
import com.google.android.libraries.feed.feedprotocoladapter.FeedProtocolAdapter;
import com.google.android.libraries.feed.feedsessionmanager.FeedSessionManager;
import com.google.android.libraries.feed.feedstore.FeedStore;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.proto.ProtoExtensionProvider;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi;
import com.google.android.libraries.feed.host.storage.ContentStorage;
import com.google.android.libraries.feed.host.storage.JournalStorage;
import com.google.android.libraries.feed.hostimpl.storage.InMemoryContentStorage;
import com.google.android.libraries.feed.hostimpl.storage.InMemoryJournalStorage;
import com.google.protobuf.GeneratedMessageLite.GeneratedExtension;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;

/**
 * This is a Scope type object which is used in the Infrastructure Integration tests. It sets the
 * Feed objects from ProtocolAdapter through the SessionManager.
 */
public class InfrastructureIntegrationScope {
  /** Scope which disables making requests when $HEAD is empty. */
  private static final SchedulerApi DISABLED_EMPTY_HEAD_REQUEST =
      new SchedulerApi() {

        @Override
        @RequestBehavior
        public int shouldSessionRequestData(SessionManagerState sessionManagerState) {
          return RequestBehavior.NO_REQUEST_WITH_CONTENT;
        }

        @Override
        public void onReceiveNewContent() {
          // Do nothing
        }

        @Override
        public void onRequestError(int networkResponseCode) {
          // Do nothing
        }
      };

  private final FeedSessionManager feedSessionManager;
  private final FeedProtocolAdapter feedProtocolAdapter;
  private final FeedModelProviderFactory modelProviderFactory;
  private final FakeRequestManager fakeRequestManager;
  private final FeedStore store;

  InfrastructureIntegrationScope(
      ThreadUtils threadUtils, ExecutorService executorService, Configuration configuration) {
    TimingUtils timingUtils = new TimingUtils();
    MainThreadRunner mainThreadRunner = new MainThreadRunner();

    FeedExtensionRegistry extensionRegistry = new FeedExtensionRegistry(new ExtensionProvider());
    ContentStorage contentStorage = new InMemoryContentStorage();
    JournalStorage journalStorage = new InMemoryJournalStorage(threadUtils);
    Clock clock = new SystemClockImpl();
    store =
        new FeedStore(
            timingUtils,
            extensionRegistry,
            contentStorage,
            journalStorage,
            threadUtils,
            mainThreadRunner,
            clock);
    feedProtocolAdapter = new FeedProtocolAdapter(timingUtils);
    fakeRequestManager = new FakeRequestManager(feedProtocolAdapter);
    feedSessionManager =
        new FeedSessionManager(
            executorService,
            store,
            timingUtils,
            threadUtils,
            feedProtocolAdapter,
            fakeRequestManager,
            DISABLED_EMPTY_HEAD_REQUEST,
            configuration,
            clock);
    modelProviderFactory =
        new FeedModelProviderFactory(
            feedSessionManager, threadUtils, timingUtils, mainThreadRunner, configuration);
  }

  public ProtocolAdapter getProtocolAdapter() {
    return feedProtocolAdapter;
  }

  public SessionManager getSessionManager() {
    return feedSessionManager;
  }

  public ModelProviderFactory getModelProviderFactory() {
    return modelProviderFactory;
  }

  public FeedStore getStore() {
    return store;
  }

  public FakeRequestManager getRequestManager() {
    return fakeRequestManager;
  }

  private static class ExtensionProvider implements ProtoExtensionProvider {
    @Override
    public List<GeneratedExtension<?, ?>> getProtoExtensions() {
      return new ArrayList<>();
    }
  }

  /** Builder for creating the {@link InfrastructureIntegrationScope} */
  public static class Builder {
    private final ThreadUtils mockThreadUtils;
    private final ExecutorService executorService;

    private Configuration configuration = new Configuration.Builder().build();

    public Builder(ThreadUtils mockThreadUtils, ExecutorService executorService) {
      this.mockThreadUtils = mockThreadUtils;
      this.executorService = executorService;
    }

    public Builder setConfiguration(Configuration configuration) {
      this.configuration = configuration;
      return this;
    }

    public InfrastructureIntegrationScope build() {
      return new InfrastructureIntegrationScope(mockThreadUtils, executorService, configuration);
    }
  }
}
