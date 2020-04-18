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

import static com.google.android.libraries.feed.api.store.Store.HEAD;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyObject;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.common.PayloadWithId;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.common.testing.ContentIdGenerators;
import com.google.android.libraries.feed.api.common.testing.InternalProtocolBuilder;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelMutation;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.requestmanager.RequestManager;
import com.google.android.libraries.feed.api.store.ContentMutation;
import com.google.android.libraries.feed.api.store.SemanticPropertiesMutation;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.protoextensions.FeedExtensionRegistry;
import com.google.android.libraries.feed.common.time.Clock;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.feedmodelprovider.FeedModelProviderFactory;
import com.google.android.libraries.feed.feedsessionmanager.internal.HeadSessionImpl;
import com.google.android.libraries.feed.feedsessionmanager.internal.Session;
import com.google.android.libraries.feed.feedstore.FeedStore;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi.RequestBehavior;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi.SessionManagerState;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.android.libraries.feed.hostimpl.storage.InMemoryContentStorage;
import com.google.android.libraries.feed.hostimpl.storage.InMemoryJournalStorage;
import com.google.common.util.concurrent.ListeningExecutorService;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.protobuf.ByteString;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamSessions;
import com.google.search.now.feed.client.StreamDataProto.StreamSharedState;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import com.google.search.now.wire.feed.PietSharedStateItemProto.PietSharedStateItem;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link FeedSessionManager} class. */
@RunWith(RobolectricTestRunner.class)
public class FeedSessionManagerTest {

  private static final MutationContext EMPTY_MUTATION = new MutationContext.Builder().build();
  private static final ContentId SHARED_STATE_ID =
      ContentId.newBuilder()
          .setContentDomain("piet-shared-state")
          .setId(1)
          .setTable("piet-shared-state")
          .build();

  private final ContentIdGenerators idGenerators = new ContentIdGenerators();
  private final String rootContentId = idGenerators.createRootContentId(0);

  private MainThreadRunner mainThreadRunner;
  @Mock private ThreadUtils threadUtils;
  @Mock private ProtocolAdapter protocolAdapter;
  @Mock private RequestManager requestManager;
  @Mock private SchedulerApi schedulerApi;
  @Mock private ModelProvider modelProvider;
  @Mock private ModelMutation modelMutation;
  @Mock private Store store;
  @Mock private Clock clock;

  @Captor private ArgumentCaptor<Consumer<Result<List<StreamDataOperation>>>> resultCapture;

  private final ContentIdGenerators contentIdGenerators = new ContentIdGenerators();
  private final TimingUtils timingUtils = new TimingUtils();
  private final FeedExtensionRegistry extensionRegistry = new FeedExtensionRegistry(ArrayList::new);
  private final Configuration configuration = new Configuration.Builder().build();

  @Before
  public void setUp() {
    initMocks(this);
    mainThreadRunner = new MainThreadRunner();
    when(store.getHeadSession()).thenReturn(HEAD);
    when(modelProvider.edit()).thenReturn(modelMutation);
    when(store.triggerContentGc(anyObject(), anyObject())).thenReturn(() -> {});
    when(store.getSharedStates()).thenReturn(Result.success(new ArrayList<>()));
    when(store.getStreamStructures(any(StreamSession.class)))
        .thenReturn(Result.success(new ArrayList<>()));
    when(store.getPayloads(any(List.class))).thenReturn(Result.success(new ArrayList<>()));
    when(store.getAllSessions()).thenReturn(Result.success(new ArrayList<>()));
    when(schedulerApi.shouldSessionRequestData(any(SessionManagerState.class)))
        .thenReturn(RequestBehavior.NO_REQUEST_WITH_CONTENT);
  }

  @Test
  public void testInitialization() {
    StreamSharedState sharedState =
        StreamSharedState.newBuilder()
            .setContentId(idGenerators.createFeatureContentId(0))
            .setPietSharedStateItem(PietSharedStateItem.getDefaultInstance())
            .build();
    List<StreamSharedState> sharedStates = new ArrayList<>();
    sharedStates.add(sharedState);
    when(store.getSharedStates()).thenReturn(Result.success(sharedStates));

    List<StreamStructure> streamStructures = new ArrayList<>();
    StreamStructure operation =
        StreamStructure.newBuilder()
            .setContentId(idGenerators.createFeatureContentId(0))
            .setOperation(StreamStructure.Operation.UPDATE_OR_APPEND)
            .build();
    streamStructures.add(operation);
    com.google.android.libraries.feed.api.store.SessionMutation sessionMutation =
        mock(com.google.android.libraries.feed.api.store.SessionMutation.class);
    when(store.getStreamStructures(any(StreamSession.class)))
        .thenReturn(Result.success(streamStructures));
    when(store.editSession(HEAD)).thenReturn(sessionMutation);
    when(sessionMutation.commit()).thenReturn(true);

    FeedSessionManager sessionManager =
        new FeedSessionManager(
            MoreExecutors.newDirectExecutorService(),
            store,
            timingUtils,
            threadUtils,
            protocolAdapter,
            requestManager,
            schedulerApi,
            configuration,
            clock);
    Map<String, StreamSharedState> sharedStateCache = sessionManager.getSharedStateCacheForTest();
    assertThat(sharedStateCache).hasSize(sharedStates.size());

    Map<String, Session> sessions = sessionManager.getSessionsMapForTest();
    Session head = sessions.get(HEAD.getStreamToken());
    assertThat(head).isInstanceOf(HeadSessionImpl.class);
    String itemKey = idGenerators.createFeatureContentId(0);
    Set<String> content = ((HeadSessionImpl) head).getContentInSessionForTest();
    assertThat(content).contains(itemKey);
    assertThat(content).hasSize(streamStructures.size());
  }

  @Test
  public void testSessionWithContent() {
    FeedSessionManager sessionManager = getInitializedSessionManager();
    int featureCnt = 3;
    populateSession(sessionManager, featureCnt, 1, true, null);

    ModelProviderFactory modelProviderFactory =
        new FeedModelProviderFactory(
            sessionManager, threadUtils, timingUtils, mainThreadRunner, configuration);
    ModelProvider modelProvider = modelProviderFactory.createNew();
    assertThat(modelProvider).isNotNull();
    assertThat(modelProvider.getRootFeature()).isNotNull();

    ModelCursor cursor = modelProvider.getRootFeature().getCursor();
    int cursorCount = 0;
    while (cursor.getNextItem() != null) {
      cursorCount++;
    }
    assertThat(cursorCount).isEqualTo(featureCnt);

    // append a couple of others
    populateSession(sessionManager, featureCnt, featureCnt + 1, false, null);

    cursor = modelProvider.getRootFeature().getCursor();
    cursorCount = 0;
    while (cursor.getNextItem() != null) {
      cursorCount++;
    }
    assertThat(cursorCount).isEqualTo(featureCnt * 2);
  }

  @Test
  public void testReset() {
    FeedSessionManager sessionManager = getInitializedSessionManager();
    int featureCnt = 3;
    int fullFeatureCount = populateSession(sessionManager, featureCnt, 1, true, null);
    assertThat(fullFeatureCount).isEqualTo(featureCnt + 1);

    fullFeatureCount = populateSession(sessionManager, featureCnt, 1, true, null);
    assertThat(fullFeatureCount).isEqualTo(featureCnt + 1);
  }

  @Test
  public void testHandleToken() {
    ByteString bytes = ByteString.copyFrom("continuation", Charset.defaultCharset());
    StreamToken streamToken =
        StreamToken.newBuilder().setNextPageToken(bytes).setParentId(rootContentId).build();
    FeedSessionManager sessionManager = getInitializedSessionManager();
    StreamSession streamSession = StreamSession.getDefaultInstance();
    sessionManager.handleToken(streamSession, streamToken);
    verify(requestManager).loadMore(eq(streamToken), resultCapture.capture());
  }

  @Test
  public void testForceRefresh() {
    StreamSession streamSession = StreamSession.newBuilder().setStreamToken("session:1").build();
    FeedSessionManager sessionManager = getInitializedSessionManager();
    sessionManager.triggerRefresh(streamSession);
    verify(requestManager)
        .triggerRefresh(eq(RequestReason.MANUAL_REFRESH), resultCapture.capture());
  }

  @Test
  public void testGetSharedState() {
    FeedSessionManager sessionManager = getInitializedSessionManager();
    String sharedStateId = idGenerators.createSharedStateContentId(0);

    populateSession(sessionManager, 3, 1, true, sharedStateId);
    when(protocolAdapter.getStreamContentId(SHARED_STATE_ID)).thenReturn(sharedStateId);
    assertThat(sessionManager.getSharedState(SHARED_STATE_ID)).isNotNull();

    // test the null condition
    ContentId undefinedSharedStateId =
        ContentId.newBuilder()
            .setContentDomain("shared-state")
            .setId(5)
            .setTable("shared-states")
            .build();
    String undefinedStreamSharedStateId =
        idGenerators.createSharedStateContentId(undefinedSharedStateId.getId());
    when(protocolAdapter.getStreamContentId(undefinedSharedStateId))
        .thenReturn(undefinedStreamSharedStateId);
    assertThat(sessionManager.getSharedState(undefinedSharedStateId)).isNull();
  }

  @Test
  public void testEdit_semanticProperties() {
    com.google.android.libraries.feed.api.store.SessionMutation session =
        mock(com.google.android.libraries.feed.api.store.SessionMutation.class);
    when(store.editSession(any(StreamSession.class))).thenReturn(session);
    ContentMutation content = mock(ContentMutation.class);
    when(store.editContent()).thenReturn(content);

    SemanticPropertiesMutation semanticPropertiesMutation = mock(SemanticPropertiesMutation.class);
    when(store.editSemanticProperties()).thenReturn(semanticPropertiesMutation);

    FeedSessionManager sessionManager =
        new FeedSessionManager(
            MoreExecutors.newDirectExecutorService(),
            store,
            timingUtils,
            threadUtils,
            protocolAdapter,
            requestManager,
            schedulerApi,
            configuration,
            clock);

    ByteString semanticData = ByteString.copyFromUtf8("helloWorld");
    StreamDataOperation streamDataOperation =
        StreamDataOperation.newBuilder()
            .setStreamPayload(StreamPayload.newBuilder().setSemanticData(semanticData))
            .setStreamStructure(StreamStructure.newBuilder().setContentId(rootContentId))
            .build();

    Consumer<Result<List<StreamDataOperation>>> updateConsumer =
        sessionManager.getUpdateConsumer(EMPTY_MUTATION);
    List<StreamDataOperation> operations = new ArrayList<>();
    operations.add(streamDataOperation);
    Result<List<StreamDataOperation>> result = Result.success(operations);
    updateConsumer.accept(result);

    verify(semanticPropertiesMutation).add(rootContentId, semanticData);
    verify(semanticPropertiesMutation).commit();
  }

  @Test
  public void testIsSessionAlive() {
    StreamSession session1 =
        StreamSession.newBuilder().setStreamToken("stream:1").setLastAccessed(0).build();
    StreamSession session2 =
        StreamSession.newBuilder()
            .setStreamToken("stream:2")
            .setLastAccessed(FeedSessionManager.DEFAULT_LIFETIME_MS - 1)
            .build();
    FeedSessionManager sessionManager =
        new FeedSessionManager(
            MoreExecutors.newDirectExecutorService(),
            store,
            timingUtils,
            threadUtils,
            protocolAdapter,
            requestManager,
            schedulerApi,
            configuration,
            clock);
    when(clock.currentTimeMillis()).thenReturn(FeedSessionManager.DEFAULT_LIFETIME_MS + 2);
    assertThat(sessionManager.isSessionAlive(session1)).isFalse();
    assertThat(sessionManager.isSessionAlive(session2)).isTrue();
  }

  @Test
  public void testGetPersistedSessions() {
    List<PayloadWithId> persistedSessions = new ArrayList<>();
    StreamSessions streamSessions = StreamSessions.getDefaultInstance();
    StreamPayload payload = StreamPayload.newBuilder().setStreamSessions(streamSessions).build();
    persistedSessions.add(new PayloadWithId(FeedSessionManager.STREAM_SESSION_CONTENT_ID, payload));
    when(store.getPayloads(any(List.class))).thenReturn(Result.success(persistedSessions));

    FeedSessionManager sessionManager =
        new FeedSessionManager(
            MoreExecutors.newDirectExecutorService(),
            store,
            timingUtils,
            threadUtils,
            protocolAdapter,
            requestManager,
            schedulerApi,
            configuration,
            clock);
    StreamSessions sessions = sessionManager.getPersistedSessions();
    assertThat(sessions).isEqualTo(streamSessions);
  }

  @Test
  public void testCleanupJournals() {
    List<StreamSession> storedSessions = new ArrayList<>();
    StreamSession session1 = StreamSession.newBuilder().setStreamToken("stream:1").build();
    StreamSession session2 = StreamSession.newBuilder().setStreamToken("stream:2").build();
    storedSessions.add(session1);
    storedSessions.add(session2);
    when(store.getAllSessions()).thenReturn(Result.success(storedSessions));

    FeedSessionManager sessionManager =
        new FeedSessionManager(
            MoreExecutors.newDirectExecutorService(),
            store,
            timingUtils,
            threadUtils,
            protocolAdapter,
            requestManager,
            schedulerApi,
            configuration,
            clock);
    Session s2 = mock(Session.class);
    when(s2.getStreamSession()).thenReturn(session2);
    sessionManager.setSessionsForTest(s2);
    sessionManager.cleanupSessionJournals();
    verify(store).removeSession(session1);
    assertThat(sessionManager.getSessionsMapForTest().get(session2.getStreamToken())).isEqualTo(s2);
  }

  @Test
  public void testInitializePersistedSessions() {
    List<PayloadWithId> persistedSessions = new ArrayList<>();
    StreamSessions.Builder streamSessionsBuilder = StreamSessions.newBuilder();
    StreamSession session1 =
        StreamSession.newBuilder().setStreamToken("stream:1").setLastAccessed(0).build();
    streamSessionsBuilder.addStreamSessions(session1);
    StreamSession session2 =
        StreamSession.newBuilder()
            .setStreamToken("stream:2")
            .setLastAccessed(FeedSessionManager.DEFAULT_LIFETIME_MS - 1)
            .build();
    streamSessionsBuilder.addStreamSessions(session2);
    StreamPayload payload =
        StreamPayload.newBuilder().setStreamSessions(streamSessionsBuilder.build()).build();
    persistedSessions.add(new PayloadWithId(FeedSessionManager.STREAM_SESSION_CONTENT_ID, payload));
    when(store.getPayloads(any(List.class))).thenReturn(Result.success(persistedSessions));

    when(clock.currentTimeMillis()).thenReturn(FeedSessionManager.DEFAULT_LIFETIME_MS + 2);
    FeedSessionManager sessionManager =
        new FeedSessionManager(
            MoreExecutors.newDirectExecutorService(),
            store,
            timingUtils,
            threadUtils,
            protocolAdapter,
            requestManager,
            schedulerApi,
            configuration,
            clock);
    sessionManager.initializePersistedSessions();
    Map<String, Session> sessionMap = sessionManager.getSessionsMapForTest();
    // Sessions will include $HEAD
    assertThat(sessionMap).hasSize(2);
    assertThat(sessionMap.get(session2.getStreamToken()).getStreamSession()).isEqualTo(session2);
    assertThat(sessionMap.get(HEAD.getStreamToken())).isNotNull();
  }

  @Test
  public void testUpdateStoredSessions() {
    FeedSessionManager sessionManager =
        new FeedSessionManager(
            MoreExecutors.newDirectExecutorService(),
            store,
            timingUtils,
            threadUtils,
            protocolAdapter,
            requestManager,
            schedulerApi,
            configuration,
            clock);
    StreamSession session1 = StreamSession.newBuilder().setStreamToken("stream:1").build();
    StreamSession session2 = StreamSession.newBuilder().setStreamToken("stream:2").build();
    Session s1 = mock(Session.class);
    when(s1.getStreamSession()).thenReturn(session1);
    Session s2 = mock(Session.class);
    when(s2.getStreamSession()).thenReturn(session2);
    sessionManager.setSessionsForTest(s1, s2);

    FakeContentMutation contentMutation = new FakeContentMutation();
    when(store.editContent()).thenReturn(contentMutation);

    sessionManager.updateStoredSessions();
    assertThat(contentMutation.commited).isTrue();
    assertThat(contentMutation.contentId).isEqualTo(FeedSessionManager.STREAM_SESSION_CONTENT_ID);
    assertThat(contentMutation.payload.hasStreamSessions()).isTrue();
    List<StreamSession> streamSessionList =
        contentMutation.payload.getStreamSessions().getStreamSessionsList();
    // There is also a $HEAD session added during initialization
    assertThat(streamSessionList).hasSize(3);
    assertThat(streamSessionList).contains(session1);
    assertThat(streamSessionList).contains(session2);
  }

  @Test
  public void testInvalidateHead() {
    /*
    StreamSession streamSession = StreamSession.newBuilder().setStreamToken("session:1").build();
    when(store.createNewSession()).thenReturn(Result.success(streamSession));
    ContentMutation content = mock(ContentMutation.class);
    when(store.editContent()).thenReturn(content);
    FeedSessionManager sessionManager =
        new FeedSessionManager(
            MoreExecutors.newDirectExecutorService(),
            store,
            timingUtils,
            threadUtils,
            protocolAdapter,
            requestManager,
            schedulerApi,
            configuration,
            clock);
    assertThat(sessionManager.getInvalidateHeadStateForTest()).isFalse();
    Map<String, Session> sessionMap = sessionManager.getSessionsMapForTest();
    assertThat(sessionMap).hasSize(1);

    sessionManager.invalidateHead();
    assertThat(sessionManager.getInvalidateHeadStateForTest()).isTrue();
    sessionManager.getNewSession(modelProvider);

    // The session hasn't been added yet.
    sessionMap = sessionManager.getSessionsMapForTest();
    assertThat(sessionMap).hasSize(1);

    List<Runnable> delayedRunnables = sessionManager.getDelayedRunnables();
    assertThat(delayedRunnables).hasSize(1);
    delayedRunnables.get(0).run();

    // The session hasn't been added yet.
    sessionMap = sessionManager.getSessionsMapForTest();
    assertThat(sessionMap).hasSize(2);
    */
  }

  private int populateSession(
      FeedSessionManager sessionManager,
      int featureCnt,
      int idStart,
      boolean reset,
      /*@Nullable*/ String sharedStateId) {
    int operationCount = 0;

    InternalProtocolBuilder internalProtocolBuilder = new InternalProtocolBuilder();
    if (reset) {
      internalProtocolBuilder.addClearOperation().addRootFeature();
      operationCount++;
    }
    for (int i = 0; i < featureCnt; i++) {
      internalProtocolBuilder.addFeature(
          contentIdGenerators.createFeatureContentId(idStart++),
          idGenerators.createRootContentId(0));
      operationCount++;
    }
    if (sharedStateId != null) {
      internalProtocolBuilder.addSharedState(sharedStateId);
      operationCount++;
    }
    Consumer<Result<List<StreamDataOperation>>> updateConsumer =
        sessionManager.getUpdateConsumer(EMPTY_MUTATION);
    List<StreamDataOperation> operations = new ArrayList<>();
    operations.addAll(internalProtocolBuilder.build());
    Result<List<StreamDataOperation>> result = Result.success(operations);
    updateConsumer.accept(result);
    return operationCount;
  }

  private FeedSessionManager getInitializedSessionManager() {
    ListeningExecutorService executor = MoreExecutors.newDirectExecutorService();
    FeedStore store =
        new FeedStore(
            timingUtils,
            extensionRegistry,
            new InMemoryContentStorage(),
            new InMemoryJournalStorage(threadUtils),
            threadUtils,
            mainThreadRunner,
            clock);
    return new FeedSessionManager(
        executor,
        store,
        timingUtils,
        threadUtils,
        protocolAdapter,
        requestManager,
        schedulerApi,
        configuration,
        clock);
  }

  private static class FakeContentMutation implements ContentMutation {
    String contentId;
    StreamPayload payload;
    boolean commited = false;

    @Override
    public ContentMutation add(String contentId, StreamPayload payload) {
      this.contentId = contentId;
      this.payload = payload;
      return this;
    }

    @Override
    public CommitResult commit() {
      commited = true;
      return CommitResult.SUCCESS;
    }
  }
}
