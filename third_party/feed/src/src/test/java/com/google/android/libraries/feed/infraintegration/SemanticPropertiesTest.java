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

package com.google.android.libraries.feed.infraintegration;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.common.SemanticPropertiesWithId;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.common.testing.ContentIdGenerators;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.testing.FakeRequestManager;
import com.google.android.libraries.feed.common.testing.InfrastructureIntegrationScope;
import com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.protobuf.ByteString;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import com.google.search.now.wire.feed.ResponseProto.Response;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests around Semantic Properties */
@RunWith(RobolectricTestRunner.class)
public class SemanticPropertiesTest {
  @Mock private ThreadUtils threadUtils;

  private FakeRequestManager requestManager;
  private SessionManager sessionManager;
  private Store store;

  @Before
  public void setUp() {
    initMocks(this);
    InfrastructureIntegrationScope scope =
        new InfrastructureIntegrationScope.Builder(
                threadUtils, MoreExecutors.newDirectExecutorService())
            .build();
    requestManager = scope.getRequestManager();
    sessionManager = scope.getSessionManager();
    store = scope.getStore();
  }

  @Test
  public void persistingSemanticProperties() {
    ContentId contentId = WireProtocolResponseBuilder.createFeatureContentId(13);
    ByteString semanticData = ByteString.copyFromUtf8("helloWorld");

    Response response =
        new WireProtocolResponseBuilder().addCardWithSemanticData(contentId, semanticData).build();
    requestManager.queueResponse(response);
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));

    ContentIdGenerators idGenerators = new ContentIdGenerators();
    String contentIdString = idGenerators.createContentId(contentId);
    Result<List<SemanticPropertiesWithId>> semanticPropertiesResult =
        store.getSemanticProperties(Collections.singletonList(contentIdString));
    assertThat(semanticPropertiesResult.isSuccessful()).isTrue();
    List<SemanticPropertiesWithId> semanticProperties = semanticPropertiesResult.getValue();
    assertThat(semanticProperties).hasSize(1);
    assertThat(semanticProperties.get(0).contentId).isEqualTo(contentIdString);
    assertThat(semanticProperties.get(0).semanticData).isEqualTo(semanticData.toByteArray());
  }
}
