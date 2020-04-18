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

package com.google.android.libraries.feed.feedrequestmanager;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.util.Base64;
import com.google.android.libraries.feed.api.actionmanager.ActionReader;
import com.google.android.libraries.feed.api.common.DismissActionWithSemanticProperties;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.protoextensions.FeedExtensionRegistry;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.network.HttpRequest;
import com.google.android.libraries.feed.host.network.HttpRequest.HttpMethod;
import com.google.android.libraries.feed.host.network.HttpResponse;
import com.google.android.libraries.feed.host.network.NetworkClient;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi;
import com.google.common.truth.extensions.proto.LiteProtoTruth;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.ExtensionRegistryLite;
import com.google.search.now.wire.feed.ActionTypeProto.ActionType;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedActionQueryDataProto.Action;
import com.google.search.now.wire.feed.FeedActionQueryDataProto.FeedActionQueryData;
import com.google.search.now.wire.feed.FeedActionQueryDataProto.FeedActionQueryDataItem;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import com.google.search.now.wire.feed.FeedRequestProto.FeedRequest;
import com.google.search.now.wire.feed.RequestProto.Request;
import com.google.search.now.wire.feed.RequestProto.Request.RequestVersion;
import com.google.search.now.wire.feed.ResponseProto.Response;
import com.google.search.now.wire.feed.SemanticPropertiesProto.SemanticProperties;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Test of the {@link FeedRequestManager} class. */
@RunWith(RobolectricTestRunner.class)
public class FeedRequestManagerTest {

  private static final int NOT_FOUND = 404;
  public static final String TABLE = "table";
  public static final String TABLE_2 = "table2";
  public static final String CONTENT_DOMAIN = "contentDomain";
  public static final String CONTENT_DOMAIN_2 = "contentDomain2";
  public static final long ID = 42;
  public static final long ID_2 = 2;
  @Mock private NetworkClient networkClient;
  @Mock private ProtocolAdapter protocolAdapter;
  @Mock private SchedulerApi scheduler;
  @Mock private ThreadUtils threadUtils;
  @Mock private ActionReader actionReader;
  @Captor private ArgumentCaptor<Consumer<HttpResponse>> responseConsumerCaptor;
  @Captor private ArgumentCaptor<HttpRequest> requestCaptor;
  @Captor private ArgumentCaptor<Response> responseCaptor;
  private ExtensionRegistryLite registry;
  private FeedRequestManager requestManager;
  private final TimingUtils timingUtils = new TimingUtils();
  private final MainThreadRunner mainThreadRunner = new MainThreadRunner();

  private final Configuration configuration = new Configuration.Builder().build();

  @Before
  public void setUp() {
    initMocks(this);
    FeedExtensionRegistry feedExtensionRegistry = new FeedExtensionRegistry(ArrayList::new);
    registry = ExtensionRegistryLite.newInstance();
    registry.add(FeedRequest.feedRequest);
    requestManager =
        new FeedRequestManager(
            configuration,
            networkClient,
            protocolAdapter,
            feedExtensionRegistry,
            scheduler,
            MoreExecutors.newDirectExecutorService(),
            timingUtils,
            threadUtils,
            mainThreadRunner,
            actionReader);

    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Collections.emptyList()));
  }

  @Test
  public void testTriggerRefresh() throws Exception {
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());

    HttpRequest httpRequest = requestCaptor.getValue();
    assertHttpRequestFormattedCorrectly(httpRequest);

    Request request =
        Request.parseFrom(
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE),
            registry);
    Request expectedRequest =
        Request.newBuilder()
            .setRequestVersion(RequestVersion.FEED_QUERY)
            .setExtension(
                FeedRequest.feedRequest,
                FeedRequest.newBuilder().setFeedQuery(FeedQuery.getDefaultInstance()).build())
            .build();
    LiteProtoTruth.assertThat(request).isEqualTo(expectedRequest);
  }

  @Test
  public void testActionData_simpleDismiss() throws Exception {
    DismissActionWithSemanticProperties dismissAction =
        buildDismissAction(ID, CONTENT_DOMAIN, TABLE, null);
    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Collections.singletonList(dismissAction)));
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());

    HttpRequest httpRequest = requestCaptor.getValue();
    assertHttpRequestFormattedCorrectly(httpRequest);

    Request request =
        Request.parseFrom(
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE),
            registry);

    List<Long> expectedIds = Collections.singletonList(ID);
    List<String> expectedContentDomains = Collections.singletonList(CONTENT_DOMAIN);
    List<String> expectedTables = Collections.singletonList(TABLE);
    List<SemanticProperties> expectedSemanticProperties = Collections.emptyList();
    List<FeedActionQueryDataItem> expectedDataItems =
        Collections.singletonList(
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(0)
                .setContentDomainIndex(0)
                .setTableIndex(0)
                .build());

    Request expectedRequest =
        Request.newBuilder()
            .setRequestVersion(RequestVersion.FEED_QUERY)
            .setExtension(
                FeedRequest.feedRequest,
                FeedRequest.newBuilder()
                    .setFeedQuery(FeedQuery.getDefaultInstance())
                    .addFeedActionQueryData(
                        FeedActionQueryData.newBuilder()
                            .setAction(Action.newBuilder().setActionType(ActionType.DISMISS))
                            .addAllUniqueId(expectedIds)
                            .addAllUniqueContentDomain(expectedContentDomains)
                            .addAllUniqueTable(expectedTables)
                            .addAllUniqueSemanticProperties(expectedSemanticProperties)
                            .addAllFeedActionQueryDataItem(expectedDataItems))
                    .build())
            .build();
    LiteProtoTruth.assertThat(request).isEqualTo(expectedRequest);
  }

  @Test
  public void testActionData_uniqueDismisses() throws Exception {
    DismissActionWithSemanticProperties dismissAction =
        buildDismissAction(ID, CONTENT_DOMAIN, TABLE, null);
    DismissActionWithSemanticProperties dismissAction2 =
        buildDismissAction(ID_2, CONTENT_DOMAIN_2, TABLE_2, null);
    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Arrays.asList(dismissAction, dismissAction2)));
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());

    HttpRequest httpRequest = requestCaptor.getValue();
    assertHttpRequestFormattedCorrectly(httpRequest);

    Request request =
        Request.parseFrom(
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE),
            registry);

    List<Long> expectedIds = Arrays.asList(ID, ID_2);
    List<String> expectedContentDomains = Arrays.asList(CONTENT_DOMAIN, CONTENT_DOMAIN_2);
    List<String> expectedTables = Arrays.asList(TABLE, TABLE_2);
    List<SemanticProperties> expectedSemanticProperties = Collections.emptyList();
    List<FeedActionQueryDataItem> expectedDataItems =
        Arrays.asList(
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(0)
                .setContentDomainIndex(0)
                .setTableIndex(0)
                .build(),
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(1)
                .setContentDomainIndex(1)
                .setTableIndex(1)
                .build());

    Request expectedRequest =
        Request.newBuilder()
            .setRequestVersion(RequestVersion.FEED_QUERY)
            .setExtension(
                FeedRequest.feedRequest,
                FeedRequest.newBuilder()
                    .setFeedQuery(FeedQuery.getDefaultInstance())
                    .addFeedActionQueryData(
                        FeedActionQueryData.newBuilder()
                            .setAction(Action.newBuilder().setActionType(ActionType.DISMISS))
                            .addAllUniqueId(expectedIds)
                            .addAllUniqueContentDomain(expectedContentDomains)
                            .addAllUniqueTable(expectedTables)
                            .addAllUniqueSemanticProperties(expectedSemanticProperties)
                            .addAllFeedActionQueryDataItem(expectedDataItems))
                    .build())
            .build();
    LiteProtoTruth.assertThat(request).isEqualTo(expectedRequest);
  }

  @Test
  public void testActionData_overlappingDismisses() throws Exception {
    DismissActionWithSemanticProperties dismissAction =
        buildDismissAction(ID, CONTENT_DOMAIN, TABLE, null);
    DismissActionWithSemanticProperties dismissAction2 =
        buildDismissAction(ID_2, CONTENT_DOMAIN, TABLE, null);
    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Arrays.asList(dismissAction, dismissAction2)));
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());

    HttpRequest httpRequest = requestCaptor.getValue();
    assertHttpRequestFormattedCorrectly(httpRequest);

    Request request =
        Request.parseFrom(
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE),
            registry);

    List<Long> expectedIds = Arrays.asList(ID, ID_2);
    List<String> expectedContentDomains = Collections.singletonList(CONTENT_DOMAIN);
    List<String> expectedTables = Collections.singletonList(TABLE);
    List<SemanticProperties> expectedSemanticProperties = Collections.emptyList();
    List<FeedActionQueryDataItem> expectedDataItems =
        Arrays.asList(
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(0)
                .setContentDomainIndex(0)
                .setTableIndex(0)
                .build(),
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(1)
                .setContentDomainIndex(0)
                .setTableIndex(0)
                .build());

    Request expectedRequest =
        Request.newBuilder()
            .setRequestVersion(RequestVersion.FEED_QUERY)
            .setExtension(
                FeedRequest.feedRequest,
                FeedRequest.newBuilder()
                    .setFeedQuery(FeedQuery.getDefaultInstance())
                    .addFeedActionQueryData(
                        FeedActionQueryData.newBuilder()
                            .setAction(Action.newBuilder().setActionType(ActionType.DISMISS))
                            .addAllUniqueId(expectedIds)
                            .addAllUniqueContentDomain(expectedContentDomains)
                            .addAllUniqueTable(expectedTables)
                            .addAllUniqueSemanticProperties(expectedSemanticProperties)
                            .addAllFeedActionQueryDataItem(expectedDataItems))
                    .build())
            .build();
    LiteProtoTruth.assertThat(request).isEqualTo(expectedRequest);
  }

  @Test
  public void testActionData_simpleDismissWithSemanticProperties() throws Exception {
    byte[] semanticPropertiesBytes = {42, 17, 88};
    DismissActionWithSemanticProperties dismissAction =
        buildDismissAction(ID, CONTENT_DOMAIN, TABLE, semanticPropertiesBytes);
    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Collections.singletonList(dismissAction)));
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());

    HttpRequest httpRequest = requestCaptor.getValue();
    assertHttpRequestFormattedCorrectly(httpRequest);

    Request request =
        Request.parseFrom(
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE),
            registry);

    List<Long> expectedIds = Collections.singletonList(ID);
    List<String> expectedContentDomains = Collections.singletonList(CONTENT_DOMAIN);
    List<String> expectedTables = Collections.singletonList(TABLE);
    List<SemanticProperties> expectedSemanticProperties =
        Collections.singletonList(
            SemanticProperties.newBuilder()
                .setSemanticPropertiesData(ByteString.copyFrom(semanticPropertiesBytes))
                .build());
    List<FeedActionQueryDataItem> expectedDataItems =
        Collections.singletonList(
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(0)
                .setContentDomainIndex(0)
                .setTableIndex(0)
                .setSemanticPropertiesIndex(0)
                .build());

    Request expectedRequest =
        Request.newBuilder()
            .setRequestVersion(RequestVersion.FEED_QUERY)
            .setExtension(
                FeedRequest.feedRequest,
                FeedRequest.newBuilder()
                    .setFeedQuery(FeedQuery.getDefaultInstance())
                    .addFeedActionQueryData(
                        FeedActionQueryData.newBuilder()
                            .setAction(Action.newBuilder().setActionType(ActionType.DISMISS))
                            .addAllUniqueId(expectedIds)
                            .addAllUniqueContentDomain(expectedContentDomains)
                            .addAllUniqueTable(expectedTables)
                            .addAllUniqueSemanticProperties(expectedSemanticProperties)
                            .addAllFeedActionQueryDataItem(expectedDataItems))
                    .build())
            .build();
    LiteProtoTruth.assertThat(request).isEqualTo(expectedRequest);
  }

  @Test
  public void testActionData_uniqueDismissesWithSemanticProperties() throws Exception {
    byte[] semanticPropertiesBytes = {42, 17, 88};
    DismissActionWithSemanticProperties dismissAction =
        buildDismissAction(ID, CONTENT_DOMAIN, TABLE, semanticPropertiesBytes);
    byte[] semanticPropertiesBytes2 = {7, 43, 91};
    DismissActionWithSemanticProperties dismissAction2 =
        buildDismissAction(ID_2, CONTENT_DOMAIN_2, TABLE_2, semanticPropertiesBytes2);
    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Arrays.asList(dismissAction, dismissAction2)));
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());

    HttpRequest httpRequest = requestCaptor.getValue();
    assertHttpRequestFormattedCorrectly(httpRequest);

    Request request =
        Request.parseFrom(
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE),
            registry);

    List<Long> expectedIds = Arrays.asList(ID, ID_2);
    List<String> expectedContentDomains = Arrays.asList(CONTENT_DOMAIN, CONTENT_DOMAIN_2);
    List<String> expectedTables = Arrays.asList(TABLE, TABLE_2);
    List<SemanticProperties> expectedSemanticProperties =
        Arrays.asList(
            SemanticProperties.newBuilder()
                .setSemanticPropertiesData(ByteString.copyFrom(semanticPropertiesBytes))
                .build(),
            SemanticProperties.newBuilder()
                .setSemanticPropertiesData(ByteString.copyFrom(semanticPropertiesBytes2))
                .build());
    List<FeedActionQueryDataItem> expectedDataItems =
        Arrays.asList(
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(0)
                .setContentDomainIndex(0)
                .setTableIndex(0)
                .setSemanticPropertiesIndex(0)
                .build(),
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(1)
                .setContentDomainIndex(1)
                .setTableIndex(1)
                .setSemanticPropertiesIndex(1)
                .build());

    Request expectedRequest =
        Request.newBuilder()
            .setRequestVersion(RequestVersion.FEED_QUERY)
            .setExtension(
                FeedRequest.feedRequest,
                FeedRequest.newBuilder()
                    .setFeedQuery(FeedQuery.getDefaultInstance())
                    .addFeedActionQueryData(
                        FeedActionQueryData.newBuilder()
                            .setAction(Action.newBuilder().setActionType(ActionType.DISMISS))
                            .addAllUniqueId(expectedIds)
                            .addAllUniqueContentDomain(expectedContentDomains)
                            .addAllUniqueTable(expectedTables)
                            .addAllUniqueSemanticProperties(expectedSemanticProperties)
                            .addAllFeedActionQueryDataItem(expectedDataItems))
                    .build())
            .build();
    LiteProtoTruth.assertThat(request).isEqualTo(expectedRequest);
  }

  @Test
  public void testActionData_overlappingDismissesWithSemanticProperties() throws Exception {
    byte[] semanticPropertiesBytes = {42, 17, 88};
    DismissActionWithSemanticProperties dismissAction =
        buildDismissAction(ID, CONTENT_DOMAIN, TABLE, semanticPropertiesBytes);
    DismissActionWithSemanticProperties dismissAction2 =
        buildDismissAction(ID_2, CONTENT_DOMAIN, TABLE_2, semanticPropertiesBytes);
    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Arrays.asList(dismissAction, dismissAction2)));
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());

    HttpRequest httpRequest = requestCaptor.getValue();
    assertHttpRequestFormattedCorrectly(httpRequest);

    Request request =
        Request.parseFrom(
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE),
            registry);

    List<Long> expectedIds = Arrays.asList(ID, ID_2);
    List<String> expectedContentDomains = Collections.singletonList(CONTENT_DOMAIN);
    List<String> expectedTables = Arrays.asList(TABLE, TABLE_2);
    List<SemanticProperties> expectedSemanticProperties =
        Collections.singletonList(
            SemanticProperties.newBuilder()
                .setSemanticPropertiesData(ByteString.copyFrom(semanticPropertiesBytes))
                .build());
    List<FeedActionQueryDataItem> expectedDataItems =
        Arrays.asList(
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(0)
                .setContentDomainIndex(0)
                .setTableIndex(0)
                .setSemanticPropertiesIndex(0)
                .build(),
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(1)
                .setContentDomainIndex(0)
                .setTableIndex(1)
                .setSemanticPropertiesIndex(0)
                .build());

    Request expectedRequest =
        Request.newBuilder()
            .setRequestVersion(RequestVersion.FEED_QUERY)
            .setExtension(
                FeedRequest.feedRequest,
                FeedRequest.newBuilder()
                    .setFeedQuery(FeedQuery.getDefaultInstance())
                    .addFeedActionQueryData(
                        FeedActionQueryData.newBuilder()
                            .setAction(Action.newBuilder().setActionType(ActionType.DISMISS))
                            .addAllUniqueId(expectedIds)
                            .addAllUniqueContentDomain(expectedContentDomains)
                            .addAllUniqueTable(expectedTables)
                            .addAllUniqueSemanticProperties(expectedSemanticProperties)
                            .addAllFeedActionQueryDataItem(expectedDataItems))
                    .build())
            .build();
    LiteProtoTruth.assertThat(request).isEqualTo(expectedRequest);
  }

  @Test
  public void testActionData_someDismissesWithSemanticProperties() throws Exception {
    DismissActionWithSemanticProperties dismissAction =
        buildDismissAction(ID, CONTENT_DOMAIN, TABLE, null);
    byte[] semanticPropertiesBytes = {42, 17, 88};
    DismissActionWithSemanticProperties dismissAction2 =
        buildDismissAction(ID_2, CONTENT_DOMAIN_2, TABLE_2, semanticPropertiesBytes);
    when(actionReader.getDismissActionsWithSemanticProperties())
        .thenReturn(Result.success(Arrays.asList(dismissAction, dismissAction2)));
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());

    HttpRequest httpRequest = requestCaptor.getValue();
    assertHttpRequestFormattedCorrectly(httpRequest);

    Request request =
        Request.parseFrom(
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE),
            registry);

    List<Long> expectedIds = Arrays.asList(ID, ID_2);
    List<String> expectedContentDomains = Arrays.asList(CONTENT_DOMAIN, CONTENT_DOMAIN_2);
    List<String> expectedTables = Arrays.asList(TABLE, TABLE_2);
    List<SemanticProperties> expectedSemanticProperties =
        Collections.singletonList(
            SemanticProperties.newBuilder()
                .setSemanticPropertiesData(ByteString.copyFrom(semanticPropertiesBytes))
                .build());
    List<FeedActionQueryDataItem> expectedDataItems =
        Arrays.asList(
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(0)
                .setContentDomainIndex(0)
                .setTableIndex(0)
                .build(),
            FeedActionQueryDataItem.newBuilder()
                .setIdIndex(1)
                .setContentDomainIndex(1)
                .setTableIndex(1)
                .setSemanticPropertiesIndex(0)
                .build());

    Request expectedRequest =
        Request.newBuilder()
            .setRequestVersion(RequestVersion.FEED_QUERY)
            .setExtension(
                FeedRequest.feedRequest,
                FeedRequest.newBuilder()
                    .setFeedQuery(FeedQuery.getDefaultInstance())
                    .addFeedActionQueryData(
                        FeedActionQueryData.newBuilder()
                            .setAction(Action.newBuilder().setActionType(ActionType.DISMISS))
                            .addAllUniqueId(expectedIds)
                            .addAllUniqueContentDomain(expectedContentDomains)
                            .addAllUniqueTable(expectedTables)
                            .addAllUniqueSemanticProperties(expectedSemanticProperties)
                            .addAllFeedActionQueryDataItem(expectedDataItems))
                    .build())
            .build();
    LiteProtoTruth.assertThat(request).isEqualTo(expectedRequest);
  }

  @Test
  public void testHandleResponse() throws Exception {
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});
    when(protocolAdapter.createModel(any(Response.class)))
        .thenReturn(Result.success(new ArrayList<>()));

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());
    Response response = Response.getDefaultInstance();
    responseConsumerCaptor.getValue().accept(createHttpResponse(/* responseCode= */ 200, response));

    verify(protocolAdapter).createModel(responseCaptor.capture());
    LiteProtoTruth.assertThat(responseCaptor.getValue()).isEqualTo(response);

    verify(scheduler).onReceiveNewContent();
  }

  @Test
  public void testHandleResponse_notFound() throws Exception {
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());
    Response response = Response.getDefaultInstance();
    responseConsumerCaptor
        .getValue()
        .accept(createHttpResponse(/* responseCode= */ NOT_FOUND, response));

    verify(scheduler).onRequestError(NOT_FOUND);
  }

  @Test
  public void testHandleResponse_missingLengthPrefixNotSupported() {
    requestManager.triggerRefresh(RequestReason.MANUAL_REFRESH, input -> {});

    verify(networkClient).send(requestCaptor.capture(), responseConsumerCaptor.capture());
    responseConsumerCaptor
        .getValue()
        .accept(
            new HttpResponse(/* responseCode= */ 200, Response.getDefaultInstance().toByteArray()));

    verify(protocolAdapter, never()).createModel(any());
  }

  private void assertHttpRequestFormattedCorrectly(HttpRequest httpRequest) {
    assertThat(httpRequest.getBody()).hasLength(0);
    assertThat(httpRequest.getMethod()).isEqualTo(HttpMethod.GET);
    assertThat(httpRequest.getUri().getQueryParameter("fmt")).isEqualTo("bin");
    assertThat(httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD))
        .isNotNull();
  }

  private HttpResponse createHttpResponse(int responseCode, Response response) throws IOException {
    byte[] rawResponse = response.toByteArray();
    ByteBuffer buffer = ByteBuffer.allocate(rawResponse.length + (Integer.SIZE / 8));
    CodedOutputStream codedOutputStream = CodedOutputStream.newInstance(buffer);
    codedOutputStream.writeUInt32NoTag(rawResponse.length);
    codedOutputStream.writeRawBytes(rawResponse);
    codedOutputStream.flush();
    byte[] newResponse = new byte[buffer.remaining()];
    buffer.get(newResponse);
    return new HttpResponse(responseCode, newResponse);
  }

  private DismissActionWithSemanticProperties buildDismissAction(
      long id, String contentDomain, String table, byte /*@Nullable*/ [] semanticProperties) {
    ContentId contentId =
        ContentId.newBuilder().setTable(table).setContentDomain(contentDomain).setId(id).build();
    return new DismissActionWithSemanticProperties(contentId, semanticProperties);
  }
}
