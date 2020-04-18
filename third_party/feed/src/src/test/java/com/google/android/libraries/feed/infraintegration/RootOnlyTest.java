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
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider.State;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderObserver;
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

/** Tests of a Stream with only a root. */
@RunWith(RobolectricTestRunner.class)
public class RootOnlyTest {
  @Mock private ThreadUtils threadUtils;

  private FakeRequestManager requestManager;
  private SessionManager sessionManager;
  private ModelProviderValidator modelValidator;
  private ModelProviderFactory modelProviderFactory;

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
  public void rootOnlyResponse_beforeSessionWithLifecycle() {
    // ModelProvider is created from $HEAD containing content
    WireProtocolResponseBuilder responseBuilder =
        new WireProtocolResponseBuilder().addClearOperation().addRootFeature();
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    ModelProvider modelProvider = modelProviderFactory.createNew();

    ModelProviderObserver changeObserver = mock(ModelProviderObserver.class);
    modelProvider.registerObserver(changeObserver);
    verify(changeObserver).onSessionStart();
    verify(changeObserver, never()).onRootSet();
    verify(changeObserver, never()).onSessionFinished();

    assertThat(modelProvider).isNotNull();
    assertThat(modelProvider.getCurrentState()).isEqualTo(State.READY);
    modelValidator.assertRoot(modelProvider);

    WireProtocolInfo protocolInfo = responseBuilder.getWireProtocolInfo();
    // 1 root
    assertThat(protocolInfo.featuresAdded).hasSize(1);
    assertThat(protocolInfo.hasClearOperation).isTrue();

    modelProvider.invalidate();
    verify(changeObserver).onSessionFinished();
  }

  @Test
  public void rootOnlyResponse_afterSessionWithLifecycle() {
    // ModelProvider created from empty $HEAD, followed by a response adding head
    // Verify the observer lifecycle is correctly called
    ModelProviderObserver changeObserver = mock(ModelProviderObserver.class);
    ModelProvider modelProvider = modelProviderFactory.createNew();
    modelProvider.registerObserver(changeObserver);
    verify(changeObserver).onSessionStart();
    verify(changeObserver, never()).onRootSet();
    verify(changeObserver, never()).onSessionFinished();

    WireProtocolResponseBuilder responseBuilder =
        new WireProtocolResponseBuilder().addRootFeature();
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    verify(changeObserver).onRootSet();
    verify(changeObserver, never()).onSessionFinished();

    assertThat(modelProvider).isNotNull();
    assertThat(modelProvider.getCurrentState()).isEqualTo(State.READY);
    modelValidator.assertRoot(modelProvider);

    WireProtocolInfo protocolInfo = responseBuilder.getWireProtocolInfo();
    // 1 root
    assertThat(protocolInfo.featuresAdded).hasSize(1);
    assertThat(protocolInfo.hasClearOperation).isFalse();

    modelProvider.invalidate();
    verify(changeObserver).onSessionFinished();
  }

  @Test
  public void rootOnlyResponse_setSecondRoot() {
    // Set the root in two different responses, verify the lifecycle is called correctly
    // and the root is replaced
    ModelProviderObserver changeObserver = mock(ModelProviderObserver.class);
    WireProtocolResponseBuilder responseBuilder =
        new WireProtocolResponseBuilder().addClearOperation().addRootFeature();
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    ModelProvider modelProvider = modelProviderFactory.createNew();
    modelProvider.registerObserver(changeObserver);
    verify(changeObserver, never()).onRootSet();
    modelValidator.assertRoot(modelProvider);

    ContentId anotherRoot =
        ContentId.newBuilder()
            .setContentDomain("root-feature")
            .setId(2)
            .setTable("feature")
            .build();
    responseBuilder = new WireProtocolResponseBuilder().addRootFeature(anotherRoot);
    requestManager.queueResponse(responseBuilder.build());
    requestManager.triggerRefresh(
        RequestReason.APP_OPEN_REFRESH,
        sessionManager.getUpdateConsumer(MutationContext.EMPTY_CONTEXT));
    verify(changeObserver).onRootSet();
    modelValidator.assertRoot(modelProvider, anotherRoot);
  }
}
