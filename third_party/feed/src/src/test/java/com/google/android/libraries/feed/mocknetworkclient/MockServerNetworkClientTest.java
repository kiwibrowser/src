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

package com.google.android.libraries.feed.mocknetworkclient;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.net.Uri;
import com.google.android.libraries.feed.api.actionmanager.ActionReader;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.protoextensions.FeedExtensionRegistry;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.feedrequestmanager.FeedRequestManager;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.network.HttpRequest;
import com.google.android.libraries.feed.host.network.HttpRequest.HttpMethod;
import com.google.android.libraries.feed.host.network.HttpResponse;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi;
import com.google.android.libraries.feed.testing.conformance.network.NetworkClientConformanceTest;
import com.google.common.truth.extensions.proto.LiteProtoTruth;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.wire.feed.ResponseProto.Response;
import com.google.search.now.wire.feed.ResponseProto.Response.ResponseVersion;
import com.google.search.now.wire.feed.mockserver.MockServerProto.ConditionalResponse;
import com.google.search.now.wire.feed.mockserver.MockServerProto.MockServer;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link MockServerNetworkClient} class. */
@RunWith(RobolectricTestRunner.class)
public class MockServerNetworkClientTest extends NetworkClientConformanceTest {

  @Mock private ProtocolAdapter protocolAdapter;
  @Mock private SchedulerApi scheduler;
  @Mock private ThreadUtils threadUtils;
  @Mock private ActionReader actionReader;
  @Captor private ArgumentCaptor<Response> responseCaptor;

  private final FeedExtensionRegistry extensionRegistry = new FeedExtensionRegistry(ArrayList::new);
  private final Configuration configuration = new Configuration.Builder().build();
  private final TimingUtils timingUtils = new TimingUtils();
  private final MainThreadRunner mainThreadRunner = new MainThreadRunner();

  @Override
  protected Uri getValidUri(@HttpMethod String method) {
    // The URI does not matter - mockNetworkClient will default to an empty response
    return new Uri.Builder().path("foo").appendPath(method).build();
  }

  @Before
  public void setUp() throws Exception {
    initMocks(this);

    MockServer mockServer = MockServer.getDefaultInstance();
    networkClient = new MockServerNetworkClient(mockServer, new Configuration.Builder().build());

    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Collections.emptyList()));
  }

  @Test
  public void testSend() throws IOException {
    MockServer.Builder mockServerBuilder = MockServer.newBuilder();
    Response initialResponse =
        Response.newBuilder().setResponseVersion(ResponseVersion.FEED_RESPONSE).build();
    mockServerBuilder.setInitialResponse(initialResponse);
    MockServerNetworkClient networkClient =
        new MockServerNetworkClient(mockServerBuilder.build(), new Configuration.Builder().build());
    Consumer<HttpResponse> responseConsumer =
        input -> {
          try {
            CodedInputStream inputStream = CodedInputStream.newInstance(input.getResponseBody());
            int length = inputStream.readRawVarint32();
            assertThat(inputStream.readRawBytes(length)).isEqualTo(initialResponse.toByteArray());
            assertThat(input.getResponseCode()).isEqualTo(200);
          } catch (IOException e) {
            throw new RuntimeException(e);
          }
        };
    networkClient.send(new HttpRequest(Uri.EMPTY, HttpMethod.POST), responseConsumer);
  }

  @Test
  public void testSend_oneTimeResponse() throws IOException {
    MockServer.Builder mockServerBuilder = MockServer.newBuilder();
    Response initialResponse =
        Response.newBuilder().setResponseVersion(ResponseVersion.FEED_RESPONSE).build();
    mockServerBuilder.setInitialResponse(initialResponse);
    MockServerNetworkClient networkClient =
        new MockServerNetworkClient(mockServerBuilder.build(), new Configuration.Builder().build());
    Consumer<HttpResponse> responseConsumer =
        input -> {
          try {
            CodedInputStream inputStream = CodedInputStream.newInstance(input.getResponseBody());
            int length = inputStream.readRawVarint32();
            assertThat(inputStream.readRawBytes(length)).isEqualTo(initialResponse.toByteArray());
            assertThat(input.getResponseCode()).isEqualTo(200);
          } catch (IOException e) {
            throw new RuntimeException(e);
          }
        };
    networkClient.send(new HttpRequest(Uri.EMPTY, HttpMethod.POST), responseConsumer);
  }

  @Test
  public void testPaging() {
    MockServer.Builder mockServerBuilder = MockServer.newBuilder();
    Response response =
        Response.newBuilder().setResponseVersion(ResponseVersion.FEED_RESPONSE).build();
    ByteString token = ByteString.copyFromUtf8("fooToken");
    StreamToken streamToken = StreamToken.newBuilder().setNextPageToken(token).build();
    ConditionalResponse.Builder conditionalResponseBuilder = ConditionalResponse.newBuilder();
    conditionalResponseBuilder.setContinuationToken(token).setResponse(response);
    mockServerBuilder.addConditionalResponses(conditionalResponseBuilder.build());
    MockServerNetworkClient networkClient =
        new MockServerNetworkClient(mockServerBuilder.build(), new Configuration.Builder().build());
    FeedRequestManager feedRequestManager =
        new FeedRequestManager(
            configuration,
            networkClient,
            protocolAdapter,
            extensionRegistry,
            scheduler,
            MoreExecutors.newDirectExecutorService(),
            timingUtils,
            threadUtils,
            mainThreadRunner,
            actionReader);
    when(protocolAdapter.createModel(any(Response.class)))
        .thenReturn(Result.success(new ArrayList<>()));

    feedRequestManager.loadMore(
        streamToken,
        result -> {
          assertThat(result.isSuccessful()).isTrue();
          assertThat(result.getValue()).hasSize(0);
        });

    verify(protocolAdapter).createModel(responseCaptor.capture());
    LiteProtoTruth.assertThat(responseCaptor.getValue()).isEqualTo(response);
  }

  @Test
  public void testPaging_noMatch() {
    MockServer.Builder mockServerBuilder = MockServer.newBuilder();
    Response response =
        Response.newBuilder().setResponseVersion(ResponseVersion.FEED_RESPONSE).build();
    // Create a MockServerConfig without a matching token.
    ConditionalResponse.Builder conditionalResponseBuilder = ConditionalResponse.newBuilder();
    conditionalResponseBuilder.setResponse(response);
    mockServerBuilder.addConditionalResponses(conditionalResponseBuilder.build());
    MockServerNetworkClient networkClient =
        new MockServerNetworkClient(mockServerBuilder.build(), new Configuration.Builder().build());
    FeedRequestManager feedRequestManager =
        new FeedRequestManager(
            configuration,
            networkClient,
            protocolAdapter,
            extensionRegistry,
            scheduler,
            MoreExecutors.newDirectExecutorService(),
            timingUtils,
            threadUtils,
            mainThreadRunner,
            actionReader);
    when(protocolAdapter.createModel(any(Response.class)))
        .thenReturn(Result.success(new ArrayList<>()));

    ByteString token = ByteString.copyFromUtf8("fooToken");
    StreamToken streamToken = StreamToken.newBuilder().setNextPageToken(token).build();
    feedRequestManager.loadMore(
        streamToken,
        result -> {
          assertThat(result.isSuccessful()).isTrue();
          assertThat(result.getValue()).hasSize(0);
        });

    verify(protocolAdapter).createModel(responseCaptor.capture());
    LiteProtoTruth.assertThat(responseCaptor.getValue()).isEqualToDefaultInstance();
  }
}
