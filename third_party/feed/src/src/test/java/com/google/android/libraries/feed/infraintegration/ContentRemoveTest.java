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
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
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

/** Tests which remove content within an existing model. */
@RunWith(RobolectricTestRunner.class)
public class ContentRemoveTest {
  @Mock private ThreadUtils threadUtils;

  private FakeRequestManager requestManager;
  private SessionManager sessionManager;
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
    modelValidator = new ModelProviderValidator(scope.getProtocolAdapter());
  }

  @Test
  public void removeContent() {
    // Create a simple stream with a root and four features
    ContentId[] cards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(1),
          WireProtocolResponseBuilder.createFeatureContentId(2),
          WireProtocolResponseBuilder.createFeatureContentId(3),
          WireProtocolResponseBuilder.createFeatureContentId(4)
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
    assertThat(modelProvider.getRootFeature()).isNotNull();
    ModelFeature rootFeature = modelProvider.getRootFeature();
    FeatureChangeObserver observer = mock(FeatureChangeObserver.class);
    rootFeature.registerObserver(observer);
    modelValidator.assertCursorSize(rootFeature.getCursor(), cards.length);

    // Create cursor advanced to each spot in the list of children
    ModelCursor advancedCursor0 = rootFeature.getCursor();
    ModelCursor advancedCursor1 = advanceCursor(rootFeature.getCursor(), 1);
    ModelCursor advancedCursor2 = advanceCursor(rootFeature.getCursor(), 2);
    ModelCursor advancedCursor3 = advanceCursor(rootFeature.getCursor(), 3);
    ModelCursor advancedCursor4 = advanceCursor(rootFeature.getCursor(), 4);

    responseBuilder = new WireProtocolResponseBuilder().removeFeature(cards[1], ROOT_CONTENT_ID);
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));

    ArgumentCaptor<FeatureChange> capture = ArgumentCaptor.forClass(FeatureChange.class);
    verify(observer).onChange(capture.capture());
    List<FeatureChange> featureChanges = capture.getAllValues();
    assertThat(featureChanges).hasSize(1);
    FeatureChange change = featureChanges.get(0);
    assertThat(change.getChildChanges().getRemovedChildren()).hasSize(1);

    modelValidator.assertCursorContents(advancedCursor0, cards[0], cards[2], cards[3]);
    modelValidator.assertCursorContents(advancedCursor1, cards[2], cards[3]);
    modelValidator.assertCursorContents(advancedCursor2, cards[2], cards[3]);
    modelValidator.assertCursorContents(advancedCursor3, cards[3]);
    modelValidator.assertCursorContents(advancedCursor4);

    // create a cursor after the remove to verify $HEAD was modified
    ModelCursor cursor = rootFeature.getCursor();
    modelValidator.assertCursorContents(cursor, cards[0], cards[2], cards[3]);
  }

  private ModelCursor advanceCursor(ModelCursor cursor, int count) {
    for (int i = 0; i < count; i++) {
      cursor.getNextItem();
    }
    return cursor;
  }
}
