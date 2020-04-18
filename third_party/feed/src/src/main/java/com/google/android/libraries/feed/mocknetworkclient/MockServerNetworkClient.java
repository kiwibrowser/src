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

import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.feedrequestmanager.FeedRequestManager;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import com.google.android.libraries.feed.host.network.HttpRequest;
import com.google.android.libraries.feed.host.network.HttpRequest.HttpMethod;
import com.google.android.libraries.feed.host.network.HttpResponse;
import com.google.android.libraries.feed.host.network.NetworkClient;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.ExtensionRegistryLite;
import com.google.search.now.wire.feed.FeedRequestProto.FeedRequest;
import com.google.search.now.wire.feed.RequestProto.Request;
import com.google.search.now.wire.feed.ResponseProto.Response;
import com.google.search.now.wire.feed.mockserver.MockServerProto.ConditionalResponse;
import com.google.search.now.wire.feed.mockserver.MockServerProto.MockServer;
import java.io.IOException;
import java.util.List;

/** A network client that returns mock responses provided via a config proto */
public class MockServerNetworkClient implements NetworkClient {
  private static final String TAG = "MockServerNetworkClient";

  private final Response initialResponse;
  private final List<ConditionalResponse> conditionalResponses;
  private final ExtensionRegistryLite extensionRegistry;
  private final long responseDelay;

  public MockServerNetworkClient(MockServer mockServer, Configuration config) {
    initialResponse = mockServer.getInitialResponse();
    conditionalResponses = mockServer.getConditionalResponsesList();

    extensionRegistry = ExtensionRegistryLite.newInstance();
    extensionRegistry.add(FeedRequest.feedRequest);
    responseDelay = config.getValueOrDefault(ConfigKey.MOCK_SERVER_DELAY_MS, 0L);
  }

  @Override
  public void send(HttpRequest httpRequest, Consumer<HttpResponse> responseConsumer) {
    try {
      Request request = getRequest(httpRequest);
      ByteString requestToken =
          (request.getExtension(FeedRequest.feedRequest).getFeedQuery().hasPageToken())
              ? request.getExtension(FeedRequest.feedRequest).getFeedQuery().getPageToken()
              : null;
      if (requestToken != null) {
        for (ConditionalResponse response : conditionalResponses) {
          if (!response.hasContinuationToken()) {
            Logger.w(TAG, "Conditional response without a token");
            continue;
          }
          if (requestToken.equals(response.getContinuationToken())) {
            delayedAccept(createHttpResponse(response.getResponse()), responseConsumer);
            return;
          }
        }
      }
      delayedAccept(createHttpResponse(initialResponse), responseConsumer);
    } catch (IOException e) {
      // TODO : handle errors here
      Logger.e(TAG, e.getMessage());
      delayedAccept(new HttpResponse(400), responseConsumer);
    }
  }

  private void delayedAccept(HttpResponse httpResponse, Consumer<HttpResponse> responseConsumer) {
    if (responseDelay <= 0) {
      responseConsumer.accept(httpResponse);
      return;
    }

    new Handler(Looper.getMainLooper())
        .postDelayed(() -> responseConsumer.accept(httpResponse), responseDelay);
  }

  private Request getRequest(HttpRequest httpRequest) throws IOException {
    byte[] rawRequest = new byte[0];
    if (httpRequest.getMethod().equals(HttpMethod.GET)) {
      if (httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD)
          != null) {
        rawRequest =
            Base64.decode(
                httpRequest.getUri().getQueryParameter(FeedRequestManager.MOTHERSHIP_PARAM_PAYLOAD),
                Base64.URL_SAFE);
      }
    } else {
      rawRequest = httpRequest.getBody();
    }
    return Request.parseFrom(rawRequest, extensionRegistry);
  }

  @Override
  public void close() {}

  private HttpResponse createHttpResponse(Response response) {
    if (response == null) {
      return new HttpResponse(500);
    }

    try {
      byte[] rawResponse = response.toByteArray();
      byte[] newResponse = new byte[rawResponse.length + (Integer.SIZE / 8)];
      CodedOutputStream codedOutputStream = CodedOutputStream.newInstance(newResponse);
      codedOutputStream.writeUInt32NoTag(rawResponse.length);
      codedOutputStream.writeRawBytes(rawResponse);
      codedOutputStream.flush();
      return new HttpResponse(200, newResponse);
    } catch (IOException e) {
      Logger.e(TAG, "Error creating response", e);
      return new HttpResponse(500);
    }
  }
}
