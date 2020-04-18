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

import static com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder.ROOT_CONTENT_ID;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.common.testing.FakeRequestManager;
import com.google.android.libraries.feed.common.testing.InfrastructureIntegrationScope;
import com.google.android.libraries.feed.common.testing.ModelProviderValidator;
import com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder;
import com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder.WireProtocolInfo;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Simple tests of a stream with multiple cards. */
@RunWith(RobolectricTestRunner.class)
public class SimpleStreamTest {
  @Mock private ThreadUtils threadUtils;

  private FakeRequestManager requestManager;
  private SessionManager sessionManager;
  private ModelProviderFactory modelProviderFactory;
  private ModelProviderValidator modelValidator;
  private ProtocolAdapter protocolAdapter;

  @Before
  public void setUp() {
    initMocks(this);
    InfrastructureIntegrationScope scope =
        new InfrastructureIntegrationScope.Builder(
                threadUtils, MoreExecutors.newDirectExecutorService())
            .build();
    requestManager = scope.getRequestManager();
    sessionManager = scope.getSessionManager();
    modelProviderFactory = scope.getModelProviderFactory();
    protocolAdapter = scope.getProtocolAdapter();
    modelValidator = new ModelProviderValidator(protocolAdapter);
  }

  @Test
  public void simpleStream_oneCard() {
    // ModelProvider created after $HEAD has content, one root, and one Card
    // A Card is two features, the Card and Content.
    WireProtocolResponseBuilder responseBuilder =
        new WireProtocolResponseBuilder()
            .addClearOperation()
            .addRootFeature()
            .addCard(WireProtocolResponseBuilder.createFeatureContentId(1), ROOT_CONTENT_ID);
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    ModelProvider modelProvider = modelProviderFactory.createNew();
    modelValidator.assertRoot(modelProvider);

    WireProtocolInfo protocolInfo = responseBuilder.getWireProtocolInfo();
    int expectedFeatureCount = 3; // 1 root, 1 Card (2 features)
    assertThat(protocolInfo.featuresAdded).hasSize(expectedFeatureCount);
    assertThat(protocolInfo.hasClearOperation).isTrue();
    modelValidator.verifyContent(modelProvider, protocolInfo.featuresAdded);

    // Validate the cursors
    ModelFeature root = modelProvider.getRootFeature();
    assertThat(root).isNotNull();
    ModelCursor cursor = root.getCursor();
    int cursorCount = 0;
    while (cursor.getNextItem() != null) {
      cursorCount++;
    }
    assertThat(cursorCount).isEqualTo(1);

    // Validate that the structure of the card
    cursor = root.getCursor();
    ModelChild child = cursor.getNextItem();
    modelValidator.assertCardStructure(child);
  }

  @Test
  public void simpleStream_twoCard() {
    // ModelProvider created after $HEAD has content, one root, and two Cards
    ContentId[] cards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(1),
          WireProtocolResponseBuilder.createFeatureContentId(2)
        };
    WireProtocolResponseBuilder responseBuilder =
        new WireProtocolResponseBuilder().addClearOperation().addRootFeature();
    for (ContentId contentId : cards) {
      responseBuilder.addCard(contentId, ROOT_CONTENT_ID);
    }
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    ModelProvider modelProvider = modelProviderFactory.createNew();
    modelValidator.assertRoot(modelProvider);

    WireProtocolInfo protocolInfo = responseBuilder.getWireProtocolInfo();
    int expectedFeatureCount = 5; // 1 root, 2 Card (* 2 features)
    assertThat(protocolInfo.featuresAdded).hasSize(expectedFeatureCount);
    assertThat(protocolInfo.hasClearOperation).isTrue();
    modelValidator.verifyContent(modelProvider, protocolInfo.featuresAdded);

    // Validate the cursor, we should have one card for each added
    ModelFeature root = modelProvider.getRootFeature();
    assertThat(root).isNotNull();
    ModelCursor cursor = root.getCursor();
    for (ContentId contentId : cards) {
      ModelChild child = cursor.getNextItem();
      assertThat(child).isNotNull();
      modelValidator.assertStreamContentId(
          child.getContentId(), protocolAdapter.getStreamContentId(contentId));
      modelValidator.assertCardStructure(child);
    }
    assertThat(cursor.isAtEnd()).isTrue();
  }
}
