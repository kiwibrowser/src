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
import com.google.android.libraries.feed.api.modelprovider.ModelChild;
import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.android.libraries.feed.api.modelprovider.ModelFeature;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider.State;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
import com.google.android.libraries.feed.api.modelprovider.TokenCompleted;
import com.google.android.libraries.feed.api.modelprovider.TokenCompletedObserver;
import com.google.android.libraries.feed.api.sessionmanager.SessionManager;
import com.google.android.libraries.feed.common.testing.FakeRequestManager;
import com.google.android.libraries.feed.common.testing.InfrastructureIntegrationScope;
import com.google.android.libraries.feed.common.testing.ModelProviderValidator;
import com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import java.util.Arrays;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Test Synthetic tokens. */
@RunWith(RobolectricTestRunner.class)
public class SyntheticTokensTest {
  private static final int INITIAL_PAGE_SIZE = 4;
  private static final int PAGE_SIZE = 4;
  private static final int MIN_PAGE_SIZE = 2;

  @Mock private ThreadUtils threadUtils;

  private FakeRequestManager requestManager;
  private SessionManager sessionManager;
  private ModelProviderFactory modelProviderFactory;
  private ModelProviderValidator modelValidator;

  @Before
  public void setUp() {
    initMocks(this);
    Configuration configuration =
        new Configuration.Builder()
            .put(ConfigKey.INITIAL_NON_CACHED_PAGE_SIZE, INITIAL_PAGE_SIZE)
            .put(ConfigKey.NON_CACHED_PAGE_SIZE, PAGE_SIZE)
            .put(ConfigKey.NON_CACHED_MIN_PAGE_SIZE, MIN_PAGE_SIZE)
            .build();
    InfrastructureIntegrationScope scope =
        new InfrastructureIntegrationScope.Builder(
                threadUtils, MoreExecutors.newDirectExecutorService())
            .setConfiguration(configuration)
            .build();
    requestManager = scope.getRequestManager();
    sessionManager = scope.getSessionManager();
    modelProviderFactory = scope.getModelProviderFactory();
    modelValidator = new ModelProviderValidator(scope.getProtocolAdapter());
  }

  /**
   * This test will test the creation of synthetic tokens.
   *
   * <ol>
   *   <li>Create an initial $HEAD with 13 items
   *   <li>Clear the FeedSessionManager ContentCache to simulate non-cached mode
   *   <li>Create a new session which will have a synthetic token at INITIAL_PAGE_SIZE
   *   <li>SessionManager.handleToken on the synthetic token, verify full cursor and partial page
   *       cursor
   *   <li>SessionManager.handleToken on the next synthetic token, verify we get PAGE_SIZE + slop.
   *       Verify both the full cursor and the partial page cursor. No further tokens will be in the
   *       cursor.
   * </ol>
   */
  @Test
  public void syntheticTokenPaging() {
    ContentId[] cards =
        new ContentId[] {
          WireProtocolResponseBuilder.createFeatureContentId(1),
          WireProtocolResponseBuilder.createFeatureContentId(2),
          WireProtocolResponseBuilder.createFeatureContentId(3),
          WireProtocolResponseBuilder.createFeatureContentId(4),
          WireProtocolResponseBuilder.createFeatureContentId(5),
          WireProtocolResponseBuilder.createFeatureContentId(6),
          WireProtocolResponseBuilder.createFeatureContentId(7),
          WireProtocolResponseBuilder.createFeatureContentId(9),
          WireProtocolResponseBuilder.createFeatureContentId(10),
          WireProtocolResponseBuilder.createFeatureContentId(11),
          WireProtocolResponseBuilder.createFeatureContentId(12),
          WireProtocolResponseBuilder.createFeatureContentId(13)
        };

    // Create 13 cards (initial page size + page size + page size and slope of 1)
    // Initial model will have all the cards
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
    assertThat(modelProvider.getCurrentState()).isEqualTo(State.READY);

    ModelFeature root = modelProvider.getRootFeature();
    assertThat(root).isNotNull();
    ModelCursor cursor = root.getCursor();
    modelValidator.assertCursorContents(cursor, cards);

    // clear the ContentCache
    clearSessionManagerContentCache();

    // Create a new ModelProvider and verify the first page size is INITIAL_PAGE_SIZE (4)
    modelProvider = modelProviderFactory.createNew();
    assertThat(modelProvider.getCurrentState()).isEqualTo(State.READY);

    root = modelProvider.getRootFeature();
    assertThat(root).isNotNull();
    cursor = root.getCursor();
    ModelChild token =
        modelValidator.assertCursorContentsWithToken(
            cursor, Arrays.copyOfRange(cards, 0, INITIAL_PAGE_SIZE));

    // Register observer, handle token, verify the full cursor and the observer's cursor
    // This should be the second page (PAGE_SIZE) and there will still be a new synthetic token
    TokenCompletedObserver tokenCompletedObserver = mock(TokenCompletedObserver.class);
    token.getModelToken().registerObserver(tokenCompletedObserver);
    modelProvider.handleToken(token.getModelToken());
    ArgumentCaptor<TokenCompleted> completedArgumentCaptor =
        ArgumentCaptor.forClass(TokenCompleted.class);
    verify(tokenCompletedObserver).onTokenCompleted(completedArgumentCaptor.capture());
    ModelCursor pageCursor = completedArgumentCaptor.getValue().getCursor();
    assertThat(pageCursor).isNotNull();
    modelValidator.assertCursorContentsWithToken(
        cursor, Arrays.copyOfRange(cards, INITIAL_PAGE_SIZE, INITIAL_PAGE_SIZE + PAGE_SIZE));
    cursor = root.getCursor();
    token =
        modelValidator.assertCursorContentsWithToken(
            cursor, Arrays.copyOfRange(cards, 0, INITIAL_PAGE_SIZE + PAGE_SIZE));

    // Register observer, handle token, verify that we pick up PAGE_SIZE + slop, verify
    // observer cursor and full cursor, no further token will be in the cursor.
    tokenCompletedObserver = mock(TokenCompletedObserver.class);
    token.getModelToken().registerObserver(tokenCompletedObserver);
    modelProvider.handleToken(token.getModelToken());
    completedArgumentCaptor = ArgumentCaptor.forClass(TokenCompleted.class);
    verify(tokenCompletedObserver).onTokenCompleted(completedArgumentCaptor.capture());
    pageCursor = completedArgumentCaptor.getValue().getCursor();
    assertThat(pageCursor).isNotNull();
    modelValidator.assertCursorContents(
        cursor, Arrays.copyOfRange(cards, INITIAL_PAGE_SIZE + PAGE_SIZE, cards.length));
    modelProvider.handleToken(token.getModelToken());
    cursor = root.getCursor();
    modelValidator.assertCursorContents(cursor, cards);
  }

  private void clearSessionManagerContentCache() {
    // This is a bit of hack, which will clear the content cache and indicate to the
    // FeedSessionManager that we are in the non-cached mode.
    requestManager.queueResponse(new WireProtocolResponseBuilder().build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
  }
}
