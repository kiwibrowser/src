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

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.common.testing.ContentIdGenerators;
import com.google.android.libraries.feed.api.common.testing.InternalProtocolBuilder;
import com.google.android.libraries.feed.api.modelprovider.ModelMutation;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.store.SessionMutation;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link SessionImpl} class. */
@RunWith(RobolectricTestRunner.class)
public class SessionImplTest {
  private static final StreamSession HEAD =
      StreamSession.newBuilder().setStreamToken("$HEAD").build();
  private static final StreamSession TEST_SESSION =
      StreamSession.newBuilder().setStreamToken("TEST$1").build();

  @Mock private Store store;
  @Mock private ModelProvider modelProvider;
  @Mock private ThreadUtils threadUtils;

  private Boolean sessionMutationResults = true;

  private final ContentIdGenerators contentIdGenerators = new ContentIdGenerators();

  private FakeSessionMutation fakeSessionMutation;
  private FakeModelMutation fakeModelMutator;
  private TaskQueue taskQueue;
  private final TimingUtils timingUtils = new TimingUtils();

  @Before
  public void setUp() {
    initMocks(this);
    when(store.getHeadSession()).thenReturn(HEAD);
    fakeSessionMutation = new FakeSessionMutation();
    when(store.editSession(TEST_SESSION)).thenReturn(fakeSessionMutation);
    fakeModelMutator = new FakeModelMutation();
    when(modelProvider.edit()).thenReturn(fakeModelMutator);
  }

  @Test
  public void testPopulateModelProvider_features() {
    SessionImpl session = getSessionImpl();
    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    // 3 features
    assertThat(streamStructures).hasSize(featureCnt);
    session.populateModelProvider(TEST_SESSION, streamStructures, false);
    assertThat(fakeModelMutator.addedChildren).hasSize(featureCnt);
    assertThat(fakeModelMutator.commitCalled).isTrue();
    assertThat(session.contentInSession).hasSize(featureCnt);
    assertThat(session.contentInSession).contains(contentIdGenerators.createFeatureContentId(1));
  }

  @Test
  public void testUpdateSession_notFullyInitialized() {
    SessionImpl session = getSessionImpl();
    assertThatRunnable(() -> session.updateSession(new ArrayList<>(), null))
        .throwsAnExceptionOfType(NullPointerException.class);
  }

  @Test
  public void testUpdateSession_features() {
    SessionImpl session = getSessionImpl();
    session.populateModelProvider(TEST_SESSION, new ArrayList<>(), false);
    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder().addClearOperation();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    // 1 clear, 3 features
    assertThat(streamStructures).hasSize(4);
    session.updateSession(streamStructures, null);
    assertThat(fakeSessionMutation.streamStructures).hasSize(featureCnt);
    assertThat(fakeModelMutator.addedChildren).hasSize(featureCnt);
    assertThat(session.contentInSession).hasSize(featureCnt);
    assertThat(session.contentInSession).contains(contentIdGenerators.createFeatureContentId(1));
  }

  @Test
  public void testUpdateFromToken() {
    SessionImpl session = getSessionImpl();
    session.populateModelProvider(TEST_SESSION, new ArrayList<>(), false);
    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    StreamToken token =
        StreamToken.newBuilder().setContentId(contentIdGenerators.createTokenContentId(2)).build();
    // The token needs to be in the session
    session.contentInSession.add(token.getContentId());
    session.updateSession(streamStructures, token);
    assertThat(fakeModelMutator.token).isEqualTo(token);
  }

  @Test
  public void testUpdateFromToken_notInSession() {
    SessionImpl session = getSessionImpl();
    session.populateModelProvider(TEST_SESSION, new ArrayList<>(), false);
    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    // The token is not in the session, so we ignore the update
    assertThat(session.contentInSession).hasSize(0);
    StreamToken token =
        StreamToken.newBuilder().setContentId(contentIdGenerators.createTokenContentId(2)).build();
    session.updateSession(streamStructures, token);
    assertThat(session.contentInSession).hasSize(0);
  }

  @Test
  public void testStorageFailure() {
    SessionImpl session = getSessionImpl();
    session.populateModelProvider(TEST_SESSION, new ArrayList<>(), false);
    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    sessionMutationResults = false;
    fakeModelMutator.clearCommit();
    session.updateSession(streamStructures, null);
    assertThat(fakeModelMutator.commitCalled).isTrue(); // Optimistic write will still call this
  }

  @Test
  public void testRemove() {
    SessionImpl session = getSessionImpl();
    session.populateModelProvider(TEST_SESSION, new ArrayList<>(), false);
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder();
    int featureCnt = 2;
    addFeatures(protocolBuilder, featureCnt, 1);
    protocolBuilder.removeFeature(
        contentIdGenerators.createFeatureContentId(1), contentIdGenerators.createRootContentId(0));
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();
    session.updateSession(streamStructures, null);
    assertThat(fakeModelMutator.removedChildren).hasSize(1);
    assertThat(session.contentInSession).hasSize(1);
  }

  private void addFeatures(InternalProtocolBuilder protocolBuilder, int featureCnt, int startId) {
    for (int i = 0; i < featureCnt; i++) {
      protocolBuilder.addFeature(
          contentIdGenerators.createFeatureContentId(startId++),
          contentIdGenerators.createRootContentId(0));
    }
  }

  private static class FakeModelMutation implements ModelMutation {
    final List<StreamStructure> addedChildren = new ArrayList<>();
    final List<StreamStructure> removedChildren = new ArrayList<>();
    final List<StreamStructure> updateChildren = new ArrayList<>();
    StreamToken token;
    boolean commitCalled = false;

    @Override
    public ModelMutation addChild(StreamStructure streamStructure) {
      addedChildren.add(streamStructure);
      return this;
    }

    @Override
    public ModelMutation removeChild(StreamStructure streamStructure) {
      removedChildren.add(streamStructure);
      return this;
    }

    @Override
    public ModelMutation updateChild(StreamStructure streamStructure) {
      updateChildren.add(streamStructure);
      return this;
    }

    @Override
    public ModelMutation setMutationSourceToken(StreamToken mutationSourceToken) {
      this.token = mutationSourceToken;
      return this;
    }

    @Override
    public ModelMutation setStreamSession(StreamSession streamSession) {
      return this;
    }

    @Override
    public ModelMutation hasCachedBindings(boolean cachedBindings) {
      return this;
    }

    @Override
    public void commit() {
      commitCalled = true;
    }

    void clearCommit() {
      commitCalled = false;
    }
  }

  private SessionImpl getSessionImpl() {
    taskQueue = new TaskQueue(MoreExecutors.newDirectExecutorService(), timingUtils);
    taskQueue.executeInitialization(() -> {});
    SessionImpl session = new SessionImpl(store, taskQueue, timingUtils, threadUtils);
    session.bindModelProvider(modelProvider);
    return session;
  }

  private class FakeSessionMutation implements SessionMutation {

    private final List<StreamStructure> streamStructures = new ArrayList<>();

    @Override
    public SessionMutation add(StreamStructure dataOperation) {
      streamStructures.add(dataOperation);
      return this;
    }

    @Override
    public Boolean commit() {
      return sessionMutationResults;
    }
  }
}
