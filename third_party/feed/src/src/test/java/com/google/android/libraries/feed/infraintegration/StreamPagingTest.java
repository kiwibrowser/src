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
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.common.testing.ContentIdGenerators;
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.api.modelprovider.TokenCompleted;
import com.google.android.libraries.feed.api.modelprovider.TokenCompletedObserver;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.common.testing.FakeRequestManager;
import com.google.android.libraries.feed.common.testing.InfrastructureIntegrationScope;
import com.google.android.libraries.feed.common.testing.ModelProviderValidator;
import com.google.android.libraries.feed.common.testing.PagingState;
import com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Test of handling paging operations within the Stream. */
@RunWith(RobolectricTestRunner.class)
public class StreamPagingTest {
  @Mock private ThreadUtils threadUtils;

  private FakeRequestManager requestManager;
  private SessionManager sessionManager;
  private ModelProviderFactory modelProviderFactory;
  private ModelProviderValidator modelValidator;
  private ContentIdGenerators contentIdGenerators = new ContentIdGenerators();

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

  /**
   * This test will create a Session and page it. Fully validating the observer and results.
   *
   * <ol>
   *   <li>Setup the initial response and the paged response
   *   <li>Setup the paging handling for the RequestManager
   *   <li>Create the Initial $HEAD from the initial response
   *   <li>Create a Session/ModelProvider
   *   <li>Setup the MutationContext and the ModelToken observer
   *   <li>ModelProvider.handleToken to cause the paging response to be loaded
   *   <li>Validate...
   * </ol>
   */
  @Test
  public void testPaging() {
    // ModelProvider created after $HEAD has content, one root, and two Cards and a token
    ContentId[] cards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(1),
          WireProtocolResponseBuilder.createFeatureContentId(2)
        };
    ContentId[] pageCards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(3),
          WireProtocolResponseBuilder.createFeatureContentId(4)
        };
    PagingState state = new PagingState(cards, pageCards, 1, contentIdGenerators);
    requestManager.queueResponse(state.initialResponse);
    requestManager.queueResponse(state.pageResponse);

    // Create an initial model
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    ModelProvider modelProvider = modelProviderFactory.createNew();
    modelValidator.assertRoot(modelProvider);

    // Validate the structure of the stream after the initial response
    ModelFeature rootFeature = modelProvider.getRootFeature();
    assertThat(rootFeature).isNotNull();
    ModelCursor cursor = rootFeature.getCursor();
    ModelChild tokenFeature = modelValidator.assertCursorContentsWithToken(cursor, cards);
    assertThat(tokenFeature).isNotNull();

    // Add an observer to the Token to get an event when the response is processed
    TokenCompletedObserver tokenCompletedObserver = mock(TokenCompletedObserver.class);
    ModelToken modelToken = tokenFeature.getModelToken();
    assertThat(modelToken).isNotNull();

    // Capture the event triggered once the paging operation is finished
    modelToken.registerObserver(tokenCompletedObserver);
    modelProvider.handleToken(modelToken);
    ArgumentCaptor<TokenCompleted> completedArgumentCaptor =
        ArgumentCaptor.forClass(TokenCompleted.class);
    verify(tokenCompletedObserver).onTokenCompleted(completedArgumentCaptor.capture());
    ModelCursor pageCursor = completedArgumentCaptor.getValue().getCursor();
    assertThat(pageCursor).isNotNull();

    // The event cursor will have only the new items added in the page
    modelValidator.assertCursorContents(pageCursor, pageCards);

    // The full cursor should now contain all the features
    cursor = rootFeature.getCursor();
    modelValidator.assertCursorContents(cursor, cards[0], cards[1], pageCards[0], pageCards[1]);
  }

  /**
   * This test will create a session and page it. Then create a new session from $HEAD to verify
   * that Head is correctly handling the token add/remove combination.
   *
   * <ul>
   *   <li>Setup the initial response and the paged response
   *   <li>Setup the paging handling for the RequestManager
   *   <li>Create the Initial $HEAD from the initial response
   *   <li>Create a Session/ModelProvider
   *   <li>Page the Session/ModelProvider
   *   <li>Create a new Session from $HEAD
   *   <li>Validate that the PageToken are not in the cursor
   * </ul>
   */
  @Test
  public void testPostPagingSessionCreation() {
    ContentId[] cards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(1),
          WireProtocolResponseBuilder.createFeatureContentId(2)
        };
    ContentId[] pageCards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(3),
          WireProtocolResponseBuilder.createFeatureContentId(4)
        };
    PagingState state = new PagingState(cards, pageCards, 1, contentIdGenerators);
    requestManager.queueResponse(state.initialResponse);
    requestManager.queueResponse(state.pageResponse);

    // Create an initial model
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    ModelProvider modelProvider = modelProviderFactory.createNew();
    ModelFeature rootFeature = modelProvider.getRootFeature();
    assertThat(rootFeature).isNotNull();
    ModelCursor cursor = rootFeature.getCursor();
    ModelChild tokenFeature = modelValidator.assertCursorContentsWithToken(cursor, cards);
    modelProvider.handleToken(tokenFeature.getModelToken());
    cursor = rootFeature.getCursor();
    modelValidator.assertCursorContents(cursor, cards[0], cards[1], pageCards[0], pageCards[1]);

    // Now create a second ModelProvider and verify the cursor.
    ModelProvider session2 = modelProviderFactory.createNew();
    rootFeature = session2.getRootFeature();
    assertThat(rootFeature).isNotNull();
    cursor = rootFeature.getCursor();
    modelValidator.assertCursorContents(cursor, cards[0], cards[1], pageCards[0], pageCards[1]);
  }
}
