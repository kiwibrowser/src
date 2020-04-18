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

import android.net.Uri;
import android.util.Base64;
import com.google.android.libraries.feed.api.actionmanager.ActionReader;
import com.google.android.libraries.feed.api.common.DismissActionWithSemanticProperties;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.requestmanager.RequestManager;
import com.google.android.libraries.feed.api.store.ActionMutation.ActionType;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.protoextensions.FeedExtensionRegistry;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import com.google.android.libraries.feed.host.network.HttpRequest;
import com.google.android.libraries.feed.host.network.HttpRequest.HttpMethod;
import com.google.android.libraries.feed.host.network.NetworkClient;
import com.google.android.libraries.feed.host.scheduler.SchedulerApi;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.wire.feed.ActionTypeProto;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.FeedActionQueryDataProto.Action;
import com.google.search.now.wire.feed.FeedActionQueryDataProto.FeedActionQueryData;
import com.google.search.now.wire.feed.FeedActionQueryDataProto.FeedActionQueryDataItem;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import com.google.search.now.wire.feed.FeedRequestProto.FeedRequest;
import com.google.search.now.wire.feed.FeedRequestProto.FeedRequest.Builder;
import com.google.search.now.wire.feed.RequestProto.Request;
import com.google.search.now.wire.feed.RequestProto.Request.RequestVersion;
import com.google.search.now.wire.feed.ResponseProto.Response;
import com.google.search.now.wire.feed.SemanticPropertiesProto.SemanticProperties;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;

/** Default implementation of RequestManager. */
public class FeedRequestManager implements RequestManager {
  public static final String MOTHERSHIP_PARAM_PAYLOAD = "reqpld";
  private static final String TAG = "JardinRM";
  private static final String MOTHERSHIP_PARAM_FORMAT = "fmt";
  private static final String MOTHERSHIP_VALUE_BINARY = "bin";

  private final Configuration configuration;
  private final NetworkClient networkClient;
  private final ProtocolAdapter protocolAdapter;
  private final FeedExtensionRegistry extensionRegistry;
  private final SchedulerApi scheduler;
  private final ExecutorService executor;
  private final TimingUtils timingUtils;
  private final ThreadUtils threadUtils;
  private final MainThreadRunner mainThreadRunner;
  private final ActionReader actionReader;

  public FeedRequestManager(
      Configuration configuration,
      NetworkClient networkClient,
      ProtocolAdapter protocolAdapter,
      FeedExtensionRegistry extensionRegistry,
      SchedulerApi scheduler,
      ExecutorService executor,
      TimingUtils timingUtils,
      ThreadUtils threadUtils,
      MainThreadRunner mainThreadRunner,
      ActionReader actionReader) {
    this.configuration = configuration;
    this.networkClient = networkClient;
    this.protocolAdapter = protocolAdapter;
    this.extensionRegistry = extensionRegistry;
    this.scheduler = scheduler;
    this.executor = executor;
    this.timingUtils = timingUtils;
    this.threadUtils = threadUtils;
    this.mainThreadRunner = mainThreadRunner;
    this.actionReader = actionReader;
  }

  @Override
  public void loadMore(
      StreamToken streamToken, Consumer<Result<List<StreamDataOperation>>> consumer) {
    threadUtils.checkMainThread();
    executor.execute(
        () -> {
          Logger.i(TAG, "Task: FeedRequestManager LoadMore");
          ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
          RequestBuilder request = newDefaultRequest().setPageToken(streamToken.getNextPageToken());
          executeRequest(request, consumer);
          timeTracker.stop(
              "task", "FeedRequestManager LoadMore", "token", streamToken.getNextPageToken());
        });
  }

  @Override
  public void triggerRefresh(
      RequestReason reason, Consumer<Result<List<StreamDataOperation>>> consumer) {
    RequestBuilder request = newDefaultRequest();
    executeRequest(request, consumer);
  }

  private RequestBuilder newDefaultRequest() {
    return new RequestBuilder();
  }

  private void executeRequest(
      RequestBuilder requestBuilder, Consumer<Result<List<StreamDataOperation>>> consumer) {
    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    if (dismissActionsResult.isSuccessful()) {
      requestBuilder.setActions(dismissActionsResult.getValue());
    } else {
      Logger.e(TAG, "Error fetching dismiss actions");
    }

    String host = configuration.getValueOrDefault(ConfigKey.FEED_SERVER_HOST, "");
    String path = configuration.getValueOrDefault(ConfigKey.FEED_SERVER_PATH_AND_PARAMS, "");
    Uri.Builder uriBuilder = Uri.parse(host + path).buildUpon();
    uriBuilder.appendQueryParameter(
        MOTHERSHIP_PARAM_PAYLOAD,
        Base64.encodeToString(
            requestBuilder.build().toByteArray(), Base64.URL_SAFE | Base64.NO_WRAP));
    uriBuilder.appendQueryParameter(MOTHERSHIP_PARAM_FORMAT, MOTHERSHIP_VALUE_BINARY);

    HttpRequest httpRequest = new HttpRequest(uriBuilder.build(), HttpMethod.GET);
    mainThreadRunner.execute(
        TAG,
        () -> {
          networkClient.send(
              httpRequest,
              input -> {
                if (input.getResponseCode() != 200) {
                  String errorBody = null;
                  try {
                    errorBody = new String(input.getResponseBody(), "UTF-8");
                  } catch (UnsupportedEncodingException e) {
                    Logger.e(TAG, "Error handling http error logging", e);
                  }
                  Logger.e(TAG, "errorCode: %d", input.getResponseCode());
                  Logger.e(TAG, "errorResponse: %s", errorBody);
                  mainThreadRunner.execute(
                      TAG,
                      () -> {
                        scheduler.onRequestError(input.getResponseCode());
                      });
                  return;
                } else {
                  mainThreadRunner.execute(TAG, scheduler::onReceiveNewContent);
                }

                handleResponseBytes(input.getResponseBody(), consumer);
              });
        });
  }

  private void handleResponseBytes(
      byte[] responseBytes, Consumer<Result<List<StreamDataOperation>>> consumer) {
    Response response;
    try {
      response =
          Response.parseFrom(
              getLengthPrefixedValue(responseBytes), extensionRegistry.getExtensionRegistry());
    } catch (IOException e) {
      Logger.e(TAG, e, "Response parse failed");
      consumer.accept(Result.failure());
      return;
    }
    Result<List<StreamDataOperation>> result = protocolAdapter.createModel(response);
    Result<List<StreamDataOperation>> contextResult;
    if (result.isSuccessful()) {
      contextResult = Result.success(result.getValue());
    } else {
      contextResult = Result.failure();
    }
    consumer.accept(contextResult);
  }

  /**
   * Returns the first length-prefixed value from {@code input}. The first bytes of input are
   * assumed to be a varint32 encoding the length of the rest of the message. If input contains more
   * than one message, only the first message is returned.i w
   *
   * @throws IOException if input cannot be parsed
   */
  private byte[] getLengthPrefixedValue(byte[] input) throws IOException {
    CodedInputStream codedInputStream = CodedInputStream.newInstance(input);
    if (codedInputStream.isAtEnd()) {
      throw new IOException("Empty length-prefixed response");
    } else {
      int length = codedInputStream.readRawVarint32();
      return codedInputStream.readRawBytes(length);
    }
  }

  // TODO: Populate ClientInfo
  private static class RequestBuilder {

    private ByteString token;
    private List<DismissActionWithSemanticProperties> dismissActionsWithSemanticProperties;

    /**
     * Sets the token used to tell the server which page of results we want in the response.
     *
     * @param token the token copied from FeedResponse.next_page_token.
     */
    RequestBuilder setPageToken(ByteString token) {
      this.token = token;
      return this;
    }

    RequestBuilder setActions(
        List<DismissActionWithSemanticProperties> dismissActionsWithSemanticProperties) {
      this.dismissActionsWithSemanticProperties = dismissActionsWithSemanticProperties;
      return this;
    }

    public Request build() {
      Request.Builder requestBuilder =
          Request.newBuilder().setRequestVersion(RequestVersion.FEED_QUERY);

      FeedQuery.Builder feedQuery = FeedQuery.newBuilder();
      if (token != null) {
        feedQuery.setPageToken(token);
      }
      Builder feedRequestBuilder = FeedRequest.newBuilder().setFeedQuery(feedQuery);
      if (dismissActionsWithSemanticProperties != null
          && !dismissActionsWithSemanticProperties.isEmpty()) {
        feedRequestBuilder.addFeedActionQueryData(buildFeedActionQueryData());
      }
      requestBuilder.setExtension(FeedRequest.feedRequest, feedRequestBuilder.build());

      return requestBuilder.build();
    }

    private FeedActionQueryData buildFeedActionQueryData() {
      Map<Long, Integer> ids = new LinkedHashMap<>(dismissActionsWithSemanticProperties.size());
      Map<String, Integer> tables =
          new LinkedHashMap<>(dismissActionsWithSemanticProperties.size());
      Map<String, Integer> contentDomains =
          new LinkedHashMap<>(dismissActionsWithSemanticProperties.size());
      Map<SemanticProperties, Integer> semanticProperties =
          new LinkedHashMap<>(dismissActionsWithSemanticProperties.size());
      ArrayList<FeedActionQueryDataItem> actionDataItems =
          new ArrayList<>(dismissActionsWithSemanticProperties.size());

      for (DismissActionWithSemanticProperties action : dismissActionsWithSemanticProperties) {
        ContentId contentId = action.getContentId();
        byte /*@Nullable*/ [] semanticPropertiesBytes = action.getSemanticProperties();

        FeedActionQueryDataItem.Builder actionDataItemBuilder =
            FeedActionQueryDataItem.newBuilder();

        actionDataItemBuilder.setIdIndex(getIndexForItem(ids, contentId.getId()));
        actionDataItemBuilder.setTableIndex(getIndexForItem(tables, contentId.getTable()));
        actionDataItemBuilder.setContentDomainIndex(
            getIndexForItem(contentDomains, contentId.getContentDomain()));
        if (semanticPropertiesBytes != null) {
          actionDataItemBuilder.setSemanticPropertiesIndex(
              getIndexForItem(
                  semanticProperties,
                  SemanticProperties.newBuilder()
                      .setSemanticPropertiesData(ByteString.copyFrom(semanticPropertiesBytes))
                      .build()));
        }

        actionDataItems.add(actionDataItemBuilder.build());
      }
      return FeedActionQueryData.newBuilder()
          .setAction(
              Action.newBuilder()
                  .setActionType(ActionTypeProto.ActionType.forNumber(ActionType.DISMISS)))
          .addAllUniqueId(ids.keySet())
          .addAllUniqueTable(tables.keySet())
          .addAllUniqueContentDomain(contentDomains.keySet())
          .addAllUniqueSemanticProperties(semanticProperties.keySet())
          .addAllFeedActionQueryDataItem(actionDataItems)
          .build();
    }

    private <T> int getIndexForItem(Map<T, Integer> objectMap, T object) {
      if (!objectMap.containsKey(object)) {
        int newIndex = objectMap.size();
        objectMap.put(object, newIndex);
        return newIndex;
      } else {
        return objectMap.get(object);
      }
    }
  }
}
