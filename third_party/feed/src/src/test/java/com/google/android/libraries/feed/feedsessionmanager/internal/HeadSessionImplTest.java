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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.testing.ContentIdGenerators;
import com.google.android.libraries.feed.api.common.testing.InternalProtocolBuilder;
import com.google.android.libraries.feed.api.store.SessionMutation;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.time.TimingUtils;
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

/** Tests of the {@link HeadSessionImpl} class. */
@RunWith(RobolectricTestRunner.class)
public class HeadSessionImplTest {
  private static final StreamSession HEAD =
      StreamSession.newBuilder().setStreamToken("$HEAD").build();

  @Mock private Store store;

  private final ContentIdGenerators contentIdGenerators = new ContentIdGenerators();
  private final TimingUtils timingUtils = new TimingUtils();
  private FakeSessionMutation fakeSessionMutation;

  @Before
  public void setUp() {
    initMocks(this);
    fakeSessionMutation = new FakeSessionMutation();
    when(store.getHeadSession()).thenReturn(HEAD);
    when(store.editSession(HEAD)).thenReturn(fakeSessionMutation);
  }

  @Test
  public void testMinimalSessionManager() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);
    assertThat(headSession.getStreamSession()).isEqualTo(store.getHeadSession());
  }

  @Test
  public void testUpdateSession_features() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);

    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder().addClearOperation();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    // 1 clear, 3 features
    assertThat(streamStructures.size()).isEqualTo(4);
    headSession.updateSession(streamStructures, null);

    // expect: 3 features
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(featureCnt);
    assertThat(headSession.contentInSession.size()).isEqualTo(featureCnt);
    assertThat(headSession.contentInSession)
        .contains(contentIdGenerators.createFeatureContentId(1));
  }

  @Test
  public void testUpdateSession_token() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);

    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder().addClearOperation();
    addFeatures(protocolBuilder, featureCnt, 1);
    protocolBuilder.addToken(contentIdGenerators.createTokenContentId(1));
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    // 1 clear, 3 features, token
    assertThat(streamStructures.size()).isEqualTo(5);
    headSession.updateSession(streamStructures, null);

    // expect: 3 features, 1 token
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(featureCnt + 1);
    assertThat(headSession.contentInSession.size()).isEqualTo(featureCnt + 1);
    assertThat(headSession.contentInSession)
        .contains(contentIdGenerators.createFeatureContentId(1));
  }

  @Test
  public void testUpdateFromToken() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);
    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    StreamToken token =
        StreamToken.newBuilder().setContentId(contentIdGenerators.createTokenContentId(2)).build();
    // The token needs to be in the session
    headSession.contentInSession.add(token.getContentId());
    headSession.updateSession(streamStructures, token);
    // features 3, plus the token added above
    assertThat(headSession.contentInSession).hasSize(featureCnt + 1);
  }

  @Test
  public void testUpdateFromToken_notInSession() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);
    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    StreamToken token =
        StreamToken.newBuilder().setContentId(contentIdGenerators.createTokenContentId(2)).build();

    // The token needs to be in the session, if not we ignore the update
    headSession.updateSession(streamStructures, token);
    assertThat(headSession.contentInSession).hasSize(0);
  }

  @Test
  public void testUpdateSession_remove() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);

    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder().addClearOperation();
    addFeatures(protocolBuilder, featureCnt, 1);
    protocolBuilder.removeFeature(
        contentIdGenerators.createFeatureContentId(1), contentIdGenerators.createRootContentId(0));
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();

    // 1 clear, 3 features, 1 remove
    assertThat(streamStructures.size()).isEqualTo(5);
    headSession.updateSession(streamStructures, null);

    // expect: 2 features (3 added, then 1 removed)
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(featureCnt + 1);
    assertThat(headSession.contentInSession.size()).isEqualTo(featureCnt - 1);
    assertThat(headSession.contentInSession)
        .contains(contentIdGenerators.createFeatureContentId(2));
    assertThat(headSession.contentInSession)
        .contains(contentIdGenerators.createFeatureContentId(3));
  }

  @Test
  public void testUpdateSession_updates() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);

    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder().addClearOperation();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();
    assertThat(streamStructures.size()).isEqualTo(4);

    // 1 clear, 3 features
    headSession.updateSession(streamStructures, null);
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(featureCnt);
    assertThat(headSession.contentInSession.size()).isEqualTo(featureCnt);

    // Now we will update feature 2
    fakeSessionMutation.streamStructures.clear();
    protocolBuilder = new InternalProtocolBuilder();
    addFeatures(protocolBuilder, 1, 2);
    streamStructures = protocolBuilder.buildAsStreamStructure();
    assertThat(streamStructures.size()).isEqualTo(1);

    // 0 features
    headSession.updateSession(streamStructures, null);
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(0);
    assertThat(headSession.contentInSession.size()).isEqualTo(featureCnt);
  }

  @Test
  public void testUpdateSession_paging() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);

    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder().addClearOperation();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();
    assertThat(streamStructures.size()).isEqualTo(4);

    // 1 clear, 3 features
    headSession.updateSession(streamStructures, null);
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(featureCnt);
    assertThat(headSession.contentInSession.size()).isEqualTo(featureCnt);

    // Now we add two new features
    fakeSessionMutation.streamStructures.clear();
    int additionalFeatureCnt = 2;
    protocolBuilder = new InternalProtocolBuilder();
    addFeatures(protocolBuilder, additionalFeatureCnt, featureCnt + 1);
    streamStructures = protocolBuilder.buildAsStreamStructure();
    assertThat(streamStructures.size()).isEqualTo(additionalFeatureCnt);

    // 0 features
    headSession.updateSession(streamStructures, null);
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(additionalFeatureCnt);
    assertThat(headSession.contentInSession.size()).isEqualTo(featureCnt + additionalFeatureCnt);
  }

  @Test
  public void testUpdateSession_clearHead() {
    HeadSessionImpl headSession = new HeadSessionImpl(store, timingUtils);

    int featureCnt = 3;
    InternalProtocolBuilder protocolBuilder = new InternalProtocolBuilder().addClearOperation();
    addFeatures(protocolBuilder, featureCnt, 1);
    List<StreamStructure> streamStructures = protocolBuilder.buildAsStreamStructure();
    assertThat(streamStructures.size()).isEqualTo(4);

    // 1 clear, 3 features
    headSession.updateSession(streamStructures, null);
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(featureCnt);
    assertThat(headSession.contentInSession.size()).isEqualTo(featureCnt);

    // Clear head and add 2 features, make sure we have new content ids
    fakeSessionMutation.streamStructures.clear();
    int newFeatureCnt = 2;
    protocolBuilder = new InternalProtocolBuilder().addClearOperation();
    addFeatures(protocolBuilder, newFeatureCnt, featureCnt + 1);
    streamStructures = protocolBuilder.buildAsStreamStructure();

    // 2 features, 1 clear
    assertThat(streamStructures.size()).isEqualTo(3);

    // 0 features
    headSession.updateSession(streamStructures, null);
    assertThat(fakeSessionMutation.streamStructures.size()).isEqualTo(newFeatureCnt);
    assertThat(headSession.contentInSession.size()).isEqualTo(newFeatureCnt);
    }

  private void addFeatures(InternalProtocolBuilder protocolBuilder, int featureCnt, int startId) {
    for (int i = 0; i < featureCnt; i++) {
      protocolBuilder.addFeature(
          contentIdGenerators.createFeatureContentId(startId++),
          contentIdGenerators.createRootContentId(0));
    }
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
      return true;
    }
  }
}
