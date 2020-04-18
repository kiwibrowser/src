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
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.FeatureChange;
import com.google.android.libraries.feed.api.modelprovider.FeatureChangeObserver;
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
import com.google.common.util.concurrent.MoreExecutors;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests which update (append) content to an existing model. */
@RunWith(RobolectricTestRunner.class)
public class StructureUpdateTest {
  @Mock private ThreadUtils threadUtils;

  private FakeRequestManager requestManager;
  private SessionManager sessionManager;
  private ProtocolAdapter protocolAdapter;
  private ModelProviderFactory modelProviderFactory;
  private ModelProviderValidator modelValidator;

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
  public void appendChildren() {
    // Create a simple stream with a root and two features
    ContentId[] startingCards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(1),
          WireProtocolResponseBuilder.createFeatureContentId(2)
        };
    // Define two features to be appended to the root
    ContentId[] appendedCards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(3),
          WireProtocolResponseBuilder.createFeatureContentId(4)
        };

    WireProtocolResponseBuilder responseBuilder =
        new WireProtocolResponseBuilder().addClearOperation().addRootFeature();
    for (ContentId contentId : startingCards) {
      responseBuilder.addCard(contentId, ROOT_CONTENT_ID);
    }
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    ModelProvider modelProvider = modelProviderFactory.createNew();

    ModelFeature root = modelProvider.getRootFeature();
    assertThat(root).isNotNull();
    FeatureChangeObserver rootObserver = mock(FeatureChangeObserver.class);
    root.registerObserver(rootObserver);
    modelValidator.assertCursorSize(root.getCursor(), startingCards.length);

    // Append new children to root
    responseBuilder = new WireProtocolResponseBuilder();
    for (ContentId contentId : appendedCards) {
      responseBuilder.addCard(contentId, ROOT_CONTENT_ID);
    }
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));

    // assert the new state of the stream
    modelValidator.assertCursorSize(root.getCursor(), startingCards.length + appendedCards.length);
    ArgumentCaptor<FeatureChange> capture = ArgumentCaptor.forClass(FeatureChange.class);
    verify(rootObserver).onChange(capture.capture());
    List<FeatureChange> featureChanges = capture.getAllValues();
    assertThat(featureChanges).hasSize(1);
    FeatureChange change = featureChanges.get(0);
    assertThat(change.getChildChanges().getAppendedChildren()).hasSize(appendedCards.length);
    assertThat(change.isFeatureChanged()).isFalse();
    int i = 0;
    for (ModelChild appendedChild : change.getChildChanges().getAppendedChildren()) {
      modelValidator.assertStreamContentId(
          appendedChild.getContentId(), protocolAdapter.getStreamContentId(appendedCards[i++]));
    }
  }

  @Test
  public void appendChildren_concurrentModification() {
    // Test which verifies the root can be updated while we advance a cursor (without
    // a ConcurrentModificationException
    // Create a simple stream with a root and two features
    ContentId[] startingCards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(1),
          WireProtocolResponseBuilder.createFeatureContentId(2)
        };
    // Define two features to be appended to the root
    ContentId[] appendedCards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(3),
          WireProtocolResponseBuilder.createFeatureContentId(4)
        };

    WireProtocolResponseBuilder responseBuilder =
        new WireProtocolResponseBuilder().addClearOperation().addRootFeature();
    for (ContentId contentId : startingCards) {
      responseBuilder.addCard(contentId, ROOT_CONTENT_ID);
    }
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    ModelProvider modelProvider = modelProviderFactory.createNew();

    ModelFeature root = modelProvider.getRootFeature();
    assertThat(root).isNotNull();
    FeatureChangeObserver rootObserver = mock(FeatureChangeObserver.class);
    root.registerObserver(rootObserver);
    ModelCursor cursor = root.getCursor();
    cursor.getNextItem();

    // Now append additional children to the stream (and cursor)
    responseBuilder = new WireProtocolResponseBuilder();
    for (ContentId contentId : appendedCards) {
      responseBuilder.addCard(contentId, ROOT_CONTENT_ID);
    }
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    modelValidator.assertCursorSize(cursor, 3);
  }
}
