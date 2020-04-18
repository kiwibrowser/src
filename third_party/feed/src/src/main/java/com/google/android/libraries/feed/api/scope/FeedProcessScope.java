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

package com.google.android.libraries.feed.api.scope;

import android.content.Context;
import com.google.android.libraries.feed.api.actionmanager.ActionManager;
import com.google.android.libraries.feed.api.actionmanager.ActionReader;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.lifecycle.AppLifecycleListener;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.requestmanager.RequestManager;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.Validators;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.protoextensions.FeedExtensionRegistry;
import com.google.android.libraries.feed.common.time.Clock;
import com.google.android.libraries.feed.common.time.SystemClockImpl;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.feedactionmanager.FeedActionManagerImpl;
import com.google.android.libraries.feed.feedactionreader.FeedActionReader;
import com.google.android.libraries.feed.feedapplifecyclelistener.FeedAppLifecycleListener;
import com.google.android.libraries.feed.feedprotocoladapter.FeedProtocolAdapter;
import com.google.android.libraries.feed.feedrequestmanager.FeedRequestManager;
import com.google.android.libraries.feed.feedsessionmanager.FeedSessionManager;
import com.google.android.libraries.feed.feedstore.FeedStore;
import com.google.android.libraries.feed.host.action.ActionApi;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.host.imageloader.ImageLoaderApi;
import com.google.android.libraries.feed.host.logging.LoggingApi;
import com.google.android.libraries.feed.host.network.NetworkClient;
import com.google.android.libraries.feed.host.proto.ProtoExtensionProvider;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi;
import com.google.android.libraries.feed.host.storage.ContentStorage;
import com.google.android.libraries.feed.host.storage.JournalStorage;
import com.google.android.libraries.feed.host.stream.CardConfiguration;
import com.google.android.libraries.feed.host.stream.StreamConfiguration;
import com.google.android.libraries.feed.hostimpl.storage.InMemoryContentStorage;
import com.google.android.libraries.feed.hostimpl.storage.InMemoryJournalStorage;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;

/**
 * Per-process instance of the feed library.
 *
 * <p>It's the host's responsibility to make sure there's only one instance of this per process, per
 * user.
 */
public class FeedProcessScope implements Dumpable {
  private static final String TAG = "FeedProcessScope";

  public FeedStreamScope.Builder createFeedStreamScopeBuilder(
      Context context,
      ActionApi actionApi,
      StreamConfiguration streamConfiguration,
      CardConfiguration cardConfiguration,
      Configuration config) {
    return new FeedStreamScope.Builder(
        context,
        actionApi,
        imageLoaderApi,
        loggingApi,
        protocolAdapter,
        sessionManager,
        threadUtils,
        timingUtils,
        mainThreadRunner,
        clock,
        debugBehavior,
        streamConfiguration,
        cardConfiguration,
        actionManager,
        config);
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    if (protocolAdapter instanceof Dumpable) {
      dumper.dump((Dumpable) protocolAdapter);
    }
    dumper.dump(timingUtils);
    if (sessionManager instanceof Dumpable) {
      dumper.dump((Dumpable) sessionManager);
    }
    if (store instanceof Dumpable) {
      dumper.dump((Dumpable) store);
    }
  }

  public void onDestroy() {
    try {
      networkClient.close();
    } catch (Exception ignored) {
      // Ignore exception when closing.
    }
  }

  /** A builder that creates a {@link FeedProcessScope}. */
  public static class Builder {

    /** The APIs are all required to construct the scope. */
    public Builder(
        Configuration configuration,
        ExecutorService singleThreadExecutor,
        ImageLoaderApi imageLoaderApi,
        LoggingApi loggingApi,
        NetworkClient networkClient,
        SchedulerApi schedulerApi,
        DebugBehavior debugBehavior) {
      this.configuration = configuration;
      this.singleThreadExecutor = singleThreadExecutor;
      this.imageLoaderApi = imageLoaderApi;
      this.loggingApi = loggingApi;
      this.networkClient = networkClient;
      this.schedulerApi = schedulerApi;
      this.debugBehavior = debugBehavior;
    }

    public Builder setProtocolAdapter(FeedProtocolAdapter protocolAdapter) {
      this.protocolAdapter = protocolAdapter;
      return this;
    }

    public Builder setRequestManager(RequestManager requestManager) {
      this.requestManager = requestManager;
      return this;
    }

    public Builder setSessionManager(SessionManager sessionManager) {
      this.sessionManager = sessionManager;
      return this;
    }

    public Builder setProtoExtensionProvider(ProtoExtensionProvider protoExtensionProvider) {
      this.protoExtensionProvider = protoExtensionProvider;
      return this;
    }

    public Builder setContentStorage(ContentStorage contentStorage) {
      this.contentStorage = contentStorage;
      return this;
    }

    public Builder setJournalStorage(JournalStorage journalStorage) {
      this.journalStorage = journalStorage;
      return this;
    }

    // This is really exposed for tests to override the thread checking
    Builder setThreadUtils(ThreadUtils threadUtils) {
      this.threadUtils = threadUtils;
      return this;
    }

    Builder setClock(Clock clock) {
      this.clock = clock;
      return this;
    }

    public FeedProcessScope build() {
      // Build default component instances if necessary.
      if (protoExtensionProvider == null) {
        // Return an empty list of extensions by default.
        protoExtensionProvider = ArrayList::new;
      }
      FeedExtensionRegistry extensionRegistry = new FeedExtensionRegistry(protoExtensionProvider);
      if (contentStorage == null) {
        contentStorage = new InMemoryContentStorage();
      }
      if (journalStorage == null) {
        journalStorage = new InMemoryJournalStorage(threadUtils);
      }
      if (clock == null) {
        clock = new SystemClockImpl();
      }
      TimingUtils timingUtils = new TimingUtils();
      MainThreadRunner mainThreadRunner = new MainThreadRunner();
      FeedStore store =
          new FeedStore(
              timingUtils,
              extensionRegistry,
              contentStorage,
              journalStorage,
              threadUtils,
              mainThreadRunner,
              clock);
      if (protocolAdapter == null) {
        protocolAdapter = new FeedProtocolAdapter(timingUtils);
      }
      ActionReader actionReader =
          new FeedActionReader(store, clock, protocolAdapter, configuration);
      if (requestManager == null) {
        requestManager =
            new FeedRequestManager(
                configuration,
                networkClient,
                protocolAdapter,
                extensionRegistry,
                schedulerApi,
                singleThreadExecutor,
                timingUtils,
                threadUtils,
                mainThreadRunner,
                actionReader);
      }
      if (sessionManager == null) {
        sessionManager =
            new FeedSessionManager(
                singleThreadExecutor,
                store,
                timingUtils,
                threadUtils,
                protocolAdapter,
                requestManager,
                schedulerApi,
                configuration,
                clock);
      }

      AppLifecycleListener appLifecycleListener = new FeedAppLifecycleListener();
      ActionManager actionManager =
          new FeedActionManagerImpl(sessionManager, store, threadUtils, singleThreadExecutor);

      return new FeedProcessScope(
          imageLoaderApi,
          loggingApi,
          networkClient,
          Validators.checkNotNull(protocolAdapter),
          Validators.checkNotNull(requestManager),
          Validators.checkNotNull(sessionManager),
          store,
          timingUtils,
          threadUtils,
          mainThreadRunner,
          appLifecycleListener,
          clock,
          debugBehavior,
          actionManager);
    }

    // Required fields.
    private final Configuration configuration;
    private final ExecutorService singleThreadExecutor;
    private final ImageLoaderApi imageLoaderApi;
    private final LoggingApi loggingApi;
    private final NetworkClient networkClient;
    private final SchedulerApi schedulerApi;
    private final DebugBehavior debugBehavior;

    // Optional fields - if they are not provided, we will use default implementations.
    /*@MonotonicNonNull*/ private RequestManager requestManager = null;
    /*@MonotonicNonNull*/ private SessionManager sessionManager = null;
    /*@MonotonicNonNull*/ private FeedProtocolAdapter protocolAdapter = null;
    /*@MonotonicNonNull*/ private ProtoExtensionProvider protoExtensionProvider = null;
    /*@MonotonicNonNull*/ private ContentStorage contentStorage = null;
    /*@MonotonicNonNull*/ private JournalStorage journalStorage = null;
    /*@MonotonicNonNull*/ private Clock clock;

    // This will be overridden in tests.
    private ThreadUtils threadUtils = new ThreadUtils();
  }

  private FeedProcessScope(
      ImageLoaderApi imageLoaderApi,
      LoggingApi loggingApi,
      NetworkClient networkClient,
      FeedProtocolAdapter protocolAdapter,
      RequestManager requestManager,
      SessionManager sessionManager,
      Store store,
      TimingUtils timingUtils,
      ThreadUtils threadUtils,
      MainThreadRunner mainThreadRunner,
      AppLifecycleListener appLifecycleListener,
      Clock clock,
      DebugBehavior debugBehavior,
      ActionManager actionManager) {
    this.imageLoaderApi = imageLoaderApi;
    this.loggingApi = loggingApi;
    this.networkClient = networkClient;
    this.protocolAdapter = protocolAdapter;
    this.requestManager = requestManager;
    this.sessionManager = sessionManager;
    this.store = store;
    this.timingUtils = timingUtils;
    this.threadUtils = threadUtils;
    this.mainThreadRunner = mainThreadRunner;
    this.appLifecycleListener = appLifecycleListener;
    this.clock = clock;
    this.debugBehavior = debugBehavior;
    this.actionManager = actionManager;
  }

  private final ImageLoaderApi imageLoaderApi;
  private final LoggingApi loggingApi;
  private final NetworkClient networkClient;
  private final ProtocolAdapter protocolAdapter;
  private final RequestManager requestManager;
  private final SessionManager sessionManager;
  private final Store store;
  private final TimingUtils timingUtils;
  private final ThreadUtils threadUtils;
  private final MainThreadRunner mainThreadRunner;
  private final AppLifecycleListener appLifecycleListener;
  private final Clock clock;
  private final DebugBehavior debugBehavior;
  private final ActionManager actionManager;

  public Clock getClock() {
    return clock;
  }

  public ProtocolAdapter getProtocolAdapter() {
    return protocolAdapter;
  }

  public RequestManager getRequestManager() {
    return requestManager;
  }

  public SessionManager getSessionManager() {
    return sessionManager;
  }

  public TimingUtils getTimingUtils() {
    return timingUtils;
  }

  public MainThreadRunner getMainThreadRunner() {
    return mainThreadRunner;
  }

  public AppLifecycleListener getAppLifecycleListener() {
    return appLifecycleListener;
  }

  public ActionManager getActionManager() {
    return actionManager;
  }
}
