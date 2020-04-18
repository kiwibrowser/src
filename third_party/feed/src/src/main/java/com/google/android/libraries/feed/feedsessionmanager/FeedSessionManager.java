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

package com.google.android.libraries.feed.feedsessionmanager;

import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.common.PayloadWithId;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.requestmanager.RequestManager;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.api.store.ContentMutation;
import com.google.android.libraries.feed.api.store.SemanticPropertiesMutation;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.functional.Supplier;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.Clock;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import com.google.android.libraries.feed.feedsessionmanager.internal.ContentCache;
import com.google.android.libraries.feed.feedsessionmanager.internal.HeadSessionImpl;
import com.google.android.libraries.feed.feedsessionmanager.internal.InitializableSession;
import com.google.android.libraries.feed.feedsessionmanager.internal.Session;
import com.google.android.libraries.feed.feedsessionmanager.internal.SessionFactory;
import com.google.android.libraries.feed.feedsessionmanager.internal.TaskQueue;
import com.google.android.libraries.feed.feedsessionmanager.internal.TaskQueue.TaskType;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi.RequestBehavior;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi.SessionManagerState;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamSessions;
import com.google.search.now.feed.client.StreamDataProto.StreamSharedState;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure.Operation;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import javax.annotation.concurrent.GuardedBy;

/** Prototype implementation of the SessionManager. All state is kept in memory. */
public class FeedSessionManager implements SessionManager, Dumpable {

  private static final String TAG = "FeedSessionManager";

  @VisibleForTesting static final String STREAM_SESSION_CONTENT_ID = "FSM::Sessions::0";
  @VisibleForTesting static final long DEFAULT_LIFETIME_MS = 3_600_000; // 1 Hour

  // Used to synchronize the stored data
  private final Object lock = new Object();

  // For the Shared State we will always cache them in the Session Manager
  @GuardedBy("lock")
  private final Map<String, StreamSharedState> sharedStateCache = new HashMap<>();

  @GuardedBy("lock")
  private final Map<String, Session> sessions = new HashMap<>();

  @GuardedBy("lock")
  private final ContentCache contentCache;

  private final SessionFactory sessionFactory;
  private final Store store;
  private final ThreadUtils threadUtils;
  private final TimingUtils timingUtils;
  private final ProtocolAdapter protocolAdapter;
  private final RequestManager requestManager;
  private final SchedulerApi schedulerApi;
  private final TaskQueue taskQueue;
  private final Clock clock;
  private final long lifetimeMs;

  // operation counts for the dumper
  private int commitCount = 0;
  private int newSessionCount = 0;
  private int existingSessionCount = 0;
  private int unboundSessionCount = 0;
  private int handleTokenCount = 0;
  private int expiredSessionsCleared = 0;

  public FeedSessionManager(
      ExecutorService executor,
      Store store,
      TimingUtils timingUtils,
      ThreadUtils threadUtils,
      ProtocolAdapter protocolAdapter,
      RequestManager requestManager,
      SchedulerApi schedulerApi,
      Configuration configuration,
      Clock clock) {
    this.store = store;
    this.timingUtils = timingUtils;
    this.threadUtils = threadUtils;
    this.protocolAdapter = protocolAdapter;
    this.requestManager = requestManager;
    this.schedulerApi = schedulerApi;
    this.clock = clock;
    this.taskQueue = new TaskQueue(executor, timingUtils);

    lifetimeMs =
        configuration.getValueOrDefault(ConfigKey.SESSION_LIFETIME_MS, DEFAULT_LIFETIME_MS);
    sessionFactory = new SessionFactory(store, taskQueue, timingUtils, threadUtils);
    contentCache = new ContentCache();

    // Suppress /*@UnderInitialization*/ warning
    @SuppressWarnings("nullness")
    Runnable initializationTask = this::initializationTask;
    taskQueue.executeInitialization(initializationTask);
  }

  // Task which initializes the Session Manager.  This must be the first task run on the
  // Session Manager thread so it's complete before we create any sessions.
  private void initializationTask() {
    Logger.i(TAG, "Task: Initialization");
    timingUtils.pinThread(Thread.currentThread(), "SessionManager");
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    // Initialize the Shared States cached here.
    ElapsedTimeTracker sharedStateTimeTracker = timingUtils.getElapsedTimeTracker(TAG);
    Result<List<StreamSharedState>> sharedStatesResult = store.getSharedStates();
    if (sharedStatesResult.isSuccessful()) {
      synchronized (lock) {
        for (StreamSharedState sharedState : sharedStatesResult.getValue()) {
          sharedStateCache.put(sharedState.getContentId(), sharedState);
        }
      }
    } else {
      // TODO: handle errors
      Logger.e(TAG, "SharedStates failed to load, no shared states are loaded.");
    }
    sharedStateTimeTracker.stop("", "sharedStateTimeTracker");

    // create the head session from the data in the Store
    ElapsedTimeTracker headTimeTracker = timingUtils.getElapsedTimeTracker(TAG);
    HeadSessionImpl head = sessionFactory.getHeadSession();
    synchronized (lock) {
      sessions.put(head.getStreamSession().getStreamToken(), head);
    }
    head.updateAccessTime(clock.currentTimeMillis());
    Result<List<StreamStructure>> streamStructuresResult =
        store.getStreamStructures(head.getStreamSession());
    List<StreamStructure> results;
    if (!streamStructuresResult.isSuccessful()) {
      Logger.i(TAG, "Unable to retrieve HEAD from Store.");
      // TODO: handle errors - We need to put things into a in-memory mode.
      results = new ArrayList<>();
    } else {
      results = streamStructuresResult.getValue();
    }
    head.initializeSession(results);
    headTimeTracker.stop("", "createHead");

    // if head is empty and the scheduler allows us, trigger refresh, otherwise live with empty head
    synchronized (lock) {
      // TODO: This code needs to be moved to the session creation.
      boolean fillingEmptyHead =
          head.isHeadEmpty()
              && schedulerApi.shouldSessionRequestData(new SessionManagerState(false, 0, false))
                  == RequestBehavior.REQUEST_WITH_WAIT;
      // Head is now initialized, unless we are filling and empty head.
      if (fillingEmptyHead) {
        taskQueue.execute(
            "refreshEmptyHead",
            TaskType.HEAD_RESET,
            () ->
                requestManager.triggerRefresh(
                    RequestReason.MANUAL_REFRESH,
                    new CommitterTask("refreshEmptyHead", MutationContext.EMPTY_CONTEXT)));
      }
    }
    initializePersistedSessions();
    timeTracker.stop("task", "Initialization");
  }

  @VisibleForTesting
  void initializePersistedSessions() {
    StreamSessions persistedSessions = getPersistedSessions();
    if (persistedSessions == null) {
      return;
    }

    boolean cleanupSessions = false;
    List<StreamSession> sessionList = persistedSessions.getStreamSessionsList();
    for (StreamSession session : sessionList) {
      if (!isSessionAlive(session)) {
        Logger.i(TAG, "Found expired session %s", session.getStreamToken());
        cleanupSessions = true;
        continue;
      }
      synchronized (lock) {
        if (sessions.containsKey(session.getStreamToken())) {
          // Don't replace sessions already found in sessions
          continue;
        }
        // Unbound sessions are sessions that are able to be created through restore
        InitializableSession unboundSession = sessionFactory.getSession();
        unboundSession.setStreamSession(session);
        sessions.put(session.getStreamToken(), unboundSession);

        // Task which populates the newly created unbound session.
        Runnable createUnboundSession =
            () -> {
              Logger.i(TAG, "Task: createUnboundSession");
              ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
              Result<List<StreamStructure>> streamStructuresResult =
                  store.getStreamStructures(session);
              if (streamStructuresResult.isSuccessful()) {
                unboundSession.populateModelProvider(
                    session, streamStructuresResult.getValue(), false);
              } else {
                // TODO: handle errors
                Logger.e(TAG, "Failed to read unbound session state, ignored");
              }
              timeTracker.stop("task", "createUnboundSession");
              unboundSessionCount++;
            };
        taskQueue.execute("createUnboundSession", TaskType.USER_FACING, createUnboundSession);
      }
    }

    if (cleanupSessions) {
      // Queue up a task to clear the session journals.
      taskQueue.execute(
          "cleanupSessionJournals", TaskType.BACKGROUND, this::cleanupSessionJournals);
    }
    Set<String> reservedContentIds = new HashSet<>();
    reservedContentIds.add(STREAM_SESSION_CONTENT_ID);
    taskQueue.execute(
        "contentGc",
        TaskType.BACKGROUND,
        store.triggerContentGc(reservedContentIds, accessibleContentSupplier));
  }

  private Supplier<Set<String>> accessibleContentSupplier =
      () -> {
        Set<String> accessibleContent = new HashSet<>();
        synchronized (lock) {
          for (Session session : sessions.values()) {
            accessibleContent.addAll(session.getContentInSession());
          }
        }
        return accessibleContent;
      };

  /**
   * Remove all session journals which are not currently found in {@code sessions} which contains
   * all of the known sessions. This is a garbage collection task.
   */
  @VisibleForTesting
  void cleanupSessionJournals() {
    Logger.i(TAG, "Task: cleanupSessionJournals");
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    int sessionCleared = expiredSessionsCleared;

    Result<List<StreamSession>> storedSessionsResult = store.getAllSessions();
    if (storedSessionsResult.isSuccessful()) {
      synchronized (lock) {
        for (StreamSession session : storedSessionsResult.getValue()) {
          if (!sessions.containsKey(session.getStreamToken())) {
            store.removeSession(session);
            expiredSessionsCleared++;
          }
        }
      }
    } else {
      // TODO: This is ignorable, but should we have a way to mark the storage layer
      // to delete all Journal files?
      Logger.e(TAG, "Error reading all sessions, Unable to cleanup session journals");
    }
    timeTracker.stop(
        "task",
        "cleanupSessionJournals",
        "sessionsCleared",
        expiredSessionsCleared - sessionCleared);
  }

  /** Returns the persisted {@link StreamSessions} or {@code null} if it's not found. */
  /*@Nullable*/
  @VisibleForTesting
  StreamSessions getPersistedSessions() {
    List<String> sessionIds = new ArrayList<>();
    sessionIds.add(STREAM_SESSION_CONTENT_ID);
    Result<List<PayloadWithId>> sessionPayloadResult = store.getPayloads(sessionIds);
    if (!sessionPayloadResult.isSuccessful()) {
      // TODO: handle errors
      return null;
    }

    List<PayloadWithId> sessionPayload = sessionPayloadResult.getValue();
    if (sessionPayload.isEmpty()) {
      Logger.w(TAG, "Persisted Sessions were not found");
      return null;
    }
    StreamPayload payload = sessionPayload.get(0).payload;
    if (!payload.hasStreamSessions()) {
      Logger.e(TAG, "Persisted Sessions StreamSessions was not set");
      return null;
    }
    return payload.getStreamSessions();
  }

  @Override
  public void getNewSession(ModelProvider modelProvider) {
    threadUtils.checkMainThread();
    InitializableSession session = sessionFactory.getSession();
    session.bindModelProvider(modelProvider);

    // Task which creates and populates the newly created session.  This must be done
    // on the Session Manager thread so it atomic with the mutations.
    Runnable populateSession =
        () -> {
          Logger.i(TAG, "Task: Create/Populate New Session");
          ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);

          Result<StreamSession> streamSessionResult = store.createNewSession();
          if (!streamSessionResult.isSuccessful()) {
            // TODO: We should invalidate the session with an error
            Logger.e(TAG, "Unable to create a new session");
            timeTracker.stop("task", "Create/Populate New Session", "Failure", "createNewSession");
            return;
          }
          StreamSession streamSession = streamSessionResult.getValue();
          StreamSession recordedSession =
              streamSession.toBuilder().setLastAccessed(clock.currentTimeMillis()).build();

          Result<List<StreamStructure>> streamStructuresResult =
              store.getStreamStructures(streamSession);
          if (!streamStructuresResult.isSuccessful()) {
            // TODO: We should invalidate the session with an error
            Logger.e(TAG, "Unable to create a new session");
            timeTracker.stop(
                "task", "Create/Populate New Session", "Failure", "getStreamStructures");
            return;
          }

          boolean cachedBindings;
          synchronized (lock) {
            cachedBindings = contentCache.size() > 0;
          }
          session.populateModelProvider(
              recordedSession, streamStructuresResult.getValue(), cachedBindings);
          synchronized (lock) {
            sessions.put(session.getStreamSession().getStreamToken(), session);
          }
          updateStoredSessions();
          newSessionCount++;
          timeTracker.stop("task", "Create/Populate New Session");
        };

    taskQueue.execute("populateNewSession", TaskType.USER_FACING, populateSession);
  }

  @VisibleForTesting
  void updateStoredSessions() {
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    StreamSessions.Builder sessionsBuilder = StreamSessions.newBuilder();
    synchronized (lock) {
      for (Session s : sessions.values()) {
        sessionsBuilder.addStreamSessions(s.getStreamSession());
      }
    }
    StreamSessions currentSessions = sessionsBuilder.build();
    StreamPayload payload = StreamPayload.newBuilder().setStreamSessions(currentSessions).build();
    ContentMutation contentMutation = store.editContent();
    contentMutation.add(STREAM_SESSION_CONTENT_ID, payload);
    contentMutation.commit();
    timeTracker.stop("task", "Update Stored Session Information");
  }

  @Override
  public boolean getExistingSession(String sessionToken, ModelProvider modelProvider) {
    threadUtils.checkMainThread();
    Session existingSession;
    synchronized (lock) {
      existingSession = sessions.get(sessionToken);
    }
    if (existingSession == null) {
      // We didn't find the existing session so it probably expired and was cleaned up.
      Logger.i(TAG, "getExistingSession didn't find the requested session %s", sessionToken);
      return false;
    }
    InitializableSession session = sessionFactory.getSession();
    session.bindModelProvider(modelProvider);
    ModelProvider existingModelProvider = existingSession.getModelProvider();
    if (existingModelProvider != null) {
      existingModelProvider.invalidate();
    }
    StreamSession streamSession = existingSession.getStreamSession();

    // Task which populates the newly created session.  This must be done
    // on the Session Manager thread so it atomic with the mutations.
    taskQueue.execute(
        "createExistingSession",
        TaskType.USER_FACING,
        () -> {
          Result<List<StreamStructure>> streamStructuresResult =
              store.getStreamStructures(streamSession);
          if (streamStructuresResult.isSuccessful()) {
            session.populateModelProvider(streamSession, streamStructuresResult.getValue(), false);
            synchronized (lock) {
              sessions.put(session.getStreamSession().getStreamToken(), session);
            }
          } else {
            // TODO: handle errors
            Logger.e(TAG, "unable to get stream structure for existing session");
          }
          existingSessionCount++;
        });
    return true;
  }

  @Override
  public void invalidateHead() {
    taskQueue.execute("invalidateHead", TaskType.HEAD_INVALIDATE, () -> resetHead(null));
  }

  @Override
  public void handleToken(StreamSession streamSession, StreamToken streamToken) {
    Logger.i(
        TAG,
        "HandleToken on stream %s, token %s",
        streamSession.getStreamToken(),
        streamToken.getContentId());
    threadUtils.checkMainThread();

    // TODO: This should only create the request once.
    handleTokenCount++;
    requestManager.loadMore(
        streamToken,
        new CommitterTask(
            "handleToken",
            new MutationContext.Builder()
                .setContinuationToken(streamToken)
                .setRequestingSession(streamSession)
                .build()));
  }

  @Override
  public void triggerRefresh(/*@Nullable*/ StreamSession streamSession) {
    threadUtils.checkMainThread();
    if (streamSession == null) {
      // see [INTERNAL LINK]  We create a new session and then call triggerRefresh on it before we have
      // even populated the session in Audrey2.
      Logger.w(TAG, "Found a null streamSession in triggerRefresh");
    }
    taskQueue.execute(
        "triggerRefresh/Request/InvalidateSession",
        TaskType.HEAD_INVALIDATE, // invalidate because we are requesting a refresh
        () -> {
          requestManager.triggerRefresh(
              RequestReason.MANUAL_REFRESH,
              new CommitterTask("triggerRefresh", MutationContext.EMPTY_CONTEXT));
          if (streamSession != null) {
            String streamToken = streamSession.getStreamToken();
            synchronized (lock) {
              Session session = sessions.get(streamToken);
              if (session != null) {
                ModelProvider modelProvider = session.getModelProvider();
                if (modelProvider != null) {
                  modelProvider.invalidate();
                } else {
                  Logger.w(TAG, "Session didn't have a ModelProvider %s", streamToken);
                }
              } else {
                Logger.w(TAG, "TriggerRefresh didn't find session %s", streamToken);
              }
            }
          }
        });
  }

  @Override
  public void getStreamFeatures(List<String> contentIds, Consumer<List<PayloadWithId>> consumer) {
    List<PayloadWithId> results = new ArrayList<>();
    List<String> cacheMisses = new ArrayList<>();
    int contentSize;
    synchronized (lock) {
      contentSize = contentCache.size();
      for (String contentId : contentIds) {
        StreamPayload payload = contentCache.get(contentId);
        if (payload != null) {
          results.add(new PayloadWithId(contentId, payload));
        } else {
          cacheMisses.add(contentId);
        }
      }
    }

    if (!cacheMisses.isEmpty()) {
      Result<List<PayloadWithId>> contentResult = store.getPayloads(cacheMisses);
      if (contentResult.isSuccessful()) {
        results.addAll(contentResult.getValue());
      } else {
        // TODO: handle errors
        Logger.e(TAG, "Unable to get the payloads in getStreamFeatures");
      }
    }
    Logger.i(
        TAG,
        "Caching getStreamFeatures - items %s, cache misses %s, cache size %s",
        contentIds.size(),
        cacheMisses.size(),
        contentSize);
    consumer.accept(results);
  }

  @Override
  /*@Nullable*/
  public StreamSharedState getSharedState(ContentId contentId) {
    threadUtils.checkMainThread();
    String sharedStateId = protocolAdapter.getStreamContentId(contentId);
    StreamSharedState state;
    synchronized (lock) {
      state = sharedStateCache.get(sharedStateId);
    }
    if (state == null) {
      Logger.e(TAG, "Shared State [%s] was not found", sharedStateId);
    }
    return state;
  }

  @Override
  public Consumer<Result<List<StreamDataOperation>>> getUpdateConsumer(
      MutationContext mutationContext) {
    return new CommitterTask("updateConsumer", mutationContext);
  }

  @Override
  public void runTask(String task, Runnable runnable) {
    taskQueue.execute(task, TaskType.USER_FACING, runnable);
  }

  @Override
  public void dump(Dumper dumper) {
    synchronized (lock) {
      dumper.title(TAG);
      dumper.forKey("commitCount").value(commitCount);
      dumper.forKey("newSessionCount").value(newSessionCount).compactPrevious();
      dumper.forKey("existingSessionCount").value(existingSessionCount).compactPrevious();
      dumper.forKey("unboundSessionCount").value(unboundSessionCount).compactPrevious();
      dumper.forKey("expiredSessionsCleared").value(expiredSessionsCleared).compactPrevious();
      dumper.forKey("handleTokenCount").value(handleTokenCount).compactPrevious();
      dumper.forKey("sharedStateCount").value(sharedStateCache.size());
      dumper.forKey("sessionCount").value(sessions.size()).compactPrevious();
      dumper.dump(contentCache);
      dumper.dump(taskQueue);
      for (Session session : sessions.values()) {
        if (session instanceof Dumpable) {
          dumper.dump((Dumpable) session);
        } else {
          dumper.forKey("session").value("Session Not Supporting Dumper");
        }
      }
    }
  }

  @VisibleForTesting
  boolean isSessionAlive(StreamSession streamSession) {
    // Today HEAD will does not time out
    return (Store.HEAD.getStreamToken().equals(streamSession.getStreamToken()))
        || ((streamSession.getLastAccessed() + lifetimeMs) > clock.currentTimeMillis());
  }

  /** This runs as a task on the executor thread and also as part of a SessionMutation commit. */
  private void resetHead(/*@Nullable*/ StreamSession streamSession) {
    Logger.i(TAG, "Task: resetHead");
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    Collection<Session> currentSessions;
    synchronized (lock) {
      currentSessions = sessions.values();
    }

    // If we have old sessions and we received a clear head, let's invalidate the session that
    // initiated the clear.
    List<String> sessionsRemoved = new ArrayList<>();
    store.clearHead();
    for (Session session : currentSessions) {
      ModelProvider modelProvider = session.getModelProvider();
      if (modelProvider != null && invalidateSession(streamSession, modelProvider)) {
        modelProvider.invalidate();
        store.removeSession(session.getStreamSession());
        sessionsRemoved.add(session.getStreamSession().getStreamToken());
      }
    }
    synchronized (lock) {
      sessions.keySet().removeAll(sessionsRemoved);
    }
    timeTracker.stop("task", "resetHead");
  }

  /**
   * This will determine if the ModelProvider (session) should be invalidated. There are three
   * situations which will cause a session to be invalidated:
   *
   * <ol>
   *   <li>Clearing head was initiated externally, not from an existing session
   *   <li>Clearing head was initiated by the ModelProvider
   *   <li>The ModelProvider doesn't yet have a session, so we'll create the session from the new
   *       $HEAD
   * </ol>
   */
  private boolean invalidateSession(
      /*@Nullable*/ StreamSession streamSession, ModelProvider modelProvider) {
    // Clear was done outside of any session.
    if (streamSession == null) {
      return true;
    }
    // Invalidate if the ModelProvider doesn't yet have a session or if it matches the session
    // which initiated the request clearing $HEAD.
    String sessionToken = modelProvider.getSessionToken();
    return (sessionToken == null || sessionToken.equals(streamSession.getStreamToken()));
  }

  /**
   * This class will process a List of {@link StreamDataOperation}. The {@link MutationContext} is
   * stored here and contains the context which made the request resulting in the List of data
   * operations.
   */
  private class CommitterTask implements Consumer<Result<List<StreamDataOperation>>> {
    private final List<StreamStructure> streamStructures = new ArrayList<>();
    private final MutationContext mutationContext;
    private final String task;

    private boolean clearedHead = false;
    private List<StreamDataOperation> dataOperations;

    CommitterTask(String task, MutationContext mutationContext) {
      this.task = task;
      this.mutationContext = mutationContext;
    }

    @Override
    public void accept(Result<List<StreamDataOperation>> updateResults) {
      if (!updateResults.isSuccessful()) {
        if (mutationContext.getContinuationToken() != null) {
          Logger.e(
              TAG, "Error found with a token request %s", mutationContext.getContinuationToken());
          // TODO: We need to route this issue through the UI for display
        } else {
          Logger.e(TAG, "Unexpected Update error found, the update is being ignored");
        }
        return;
      }
      dataOperations = updateResults.getValue();
      for (StreamDataOperation operation : dataOperations) {
        if (operation.getStreamStructure().getOperation() == Operation.CLEAR_ALL) {
          clearedHead = true;
          break;
        }
      }
      Logger.i("TaskQueue", "CommitterTask is clearing head %s", clearedHead);
      taskQueue.execute(
          "sessionManagerMutation:" + task,
          clearedHead ? TaskType.HEAD_RESET : TaskType.USER_FACING,
          this::commitTask);
    }

    private void commitTask() {
      synchronized (lock) {
        contentCache.startMutation();
      }
      ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
      commitContent();
      // TODO: If we are currently delaying creation and we didn't clear head, then this
      // is an update.  Should we delay commitSessionUpdates until head is population?
      commitSessionUpdates();
      commitCount++;
      synchronized (lock) {
        contentCache.finishMutation();
      }
      timeTracker.stop(
          "task", "sessionMutation.commitTask:" + task, "mutations", streamStructures.size());
    }

    private void commitContent() {
      ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);

      ContentMutation contentMutation = store.editContent();
      SemanticPropertiesMutation semanticPropertiesMutation = store.editSemanticProperties();
      for (StreamDataOperation dataOperation : dataOperations) {
        Operation operation = dataOperation.getStreamStructure().getOperation();
        if (operation == Operation.CLEAR_ALL) {
          streamStructures.add(dataOperation.getStreamStructure());
          resetHead((mutationContext != null) ? mutationContext.getRequestingSession() : null);
          continue;
        }
        if (operation == Operation.UPDATE_OR_APPEND) {
          if (!validDataOperation(dataOperation)) {
            continue;
          }
          StreamPayload payload = dataOperation.getStreamPayload();
          String contentId = dataOperation.getStreamStructure().getContentId();
          synchronized (lock) {
            contentCache.put(contentId, payload);
          }
          if (payload.hasStreamSharedState()) {
            contentMutation.add(contentId, payload);
            synchronized (lock) {
              // cache the shared state here
              sharedStateCache.put(
                  dataOperation.getStreamStructure().getContentId(),
                  payload.getStreamSharedState());
            }
          } else if (payload.hasStreamFeature() || payload.hasStreamToken()) {
            // don't add StreamSharedState to the metadata list stored for sessions
            streamStructures.add(dataOperation.getStreamStructure());
            contentMutation.add(contentId, payload);
          } else {
            Logger.e(TAG, "Unsupported UPDATE_OR_APPEND payload");
          }
          continue;
        } else if (operation == Operation.REMOVE) {
          // We don't update the content for REMOVED items, content will be garbage collected.
          // This is required because a session may not remove the item.
          // Add the Remove to the structure changes
          streamStructures.add(dataOperation.getStreamStructure());
          continue;
        }
        if (dataOperation.getStreamPayload().hasSemanticData()) {
          String contentId = dataOperation.getStreamStructure().getContentId();
          semanticPropertiesMutation.add(
              contentId, dataOperation.getStreamPayload().getSemanticData());
          continue;
        }
        Logger.e(
            TAG, "Unsupported Mutation: %s", dataOperation.getStreamStructure().getOperation());
      }
      taskQueue.execute("contentMutation", TaskType.BACKGROUND, contentMutation::commit);
      // TODO: currently there is a small window in which an action could occur and the
      // request be sent to the server before this is written, fix
      taskQueue.execute(
          "semanticPropertiesMutation", TaskType.BACKGROUND, semanticPropertiesMutation::commit);
      timeTracker.stop("", "contentUpdate", "items", dataOperations.size());
    }

    private void commitSessionUpdates() {
      ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
      ArrayList<Session> updates;
      /*@Nullable*/
      StreamToken mutationSourceToken =
          (mutationContext != null) ? mutationContext.getContinuationToken() : null;
      synchronized (lock) {
        // For sessions we want to add the remove operation if the mutation source was a
        // continuation token.
        if (mutationSourceToken != null) {
          StreamStructure removeOperation = addTokenRemoveOperation(mutationSourceToken);
          if (removeOperation != null) {
            streamStructures.add(0, removeOperation);
          }
        }
        updates = new ArrayList<>(sessions.values());
      }
      for (Session session : updates) {
        // TODO: Add something to the interface to avoid the instanceof?
        if (clearedHead && !(session instanceof HeadSessionImpl)) {
          // if we cleared the head, all existing session will not be updated by this mutation
          continue;
        }
        Logger.i(TAG, "Update Session %s", session.getStreamSession().getStreamToken());
        session.updateSession(streamStructures, mutationSourceToken);
      }
      timeTracker.stop("", "sessionUpdate", "sessions", updates.size());
    }

    private boolean validDataOperation(StreamDataOperation dataOperation) {
      if (!dataOperation.hasStreamPayload() || !dataOperation.hasStreamStructure()) {
        Logger.e(TAG, "Invalid StreamDataOperation - payload or streamStructure not defined");
        return false;
      }
      String contentId = dataOperation.getStreamStructure().getContentId();
      if (TextUtils.isEmpty(contentId)) {
        Logger.i(TAG, "Invalid StreamDataOperation - No ID Found");
        return false;
      }
      return true;
    }

    /*@Nullable*/
    private StreamStructure addTokenRemoveOperation(StreamToken token) {
      return StreamStructure.newBuilder()
          .setContentId(token.getContentId())
          .setParentContentId(token.getParentId())
          .setOperation(Operation.REMOVE)
          .build();
    }
  }

  // This is only used in tests to verify the contents of the shared state cache.
  Map<String, StreamSharedState> getSharedStateCacheForTest() {
    synchronized (lock) {
      return new HashMap<>(sharedStateCache);
    }
  }

  // This is only used in tests to access a copy of the session state
  Map<String, Session> getSessionsMapForTest() {
    synchronized (lock) {
      return new HashMap<>(sessions);
    }
  }

  void setSessionsForTest(Session... testSessions) {
    synchronized (lock) {
      for (Session session : testSessions) {
        sessions.put(session.getStreamSession().getStreamToken(), session);
      }
    }
  }
}
