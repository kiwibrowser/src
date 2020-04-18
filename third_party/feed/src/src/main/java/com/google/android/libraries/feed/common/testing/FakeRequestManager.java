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

package com.google.android.libraries.feed.common.testing;

import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.requestmanager.RequestManager;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.wire.feed.FeedQueryProto.FeedQuery.RequestReason;
import com.google.search.now.wire.feed.ResponseProto.Response;
import java.util.ArrayDeque;
import java.util.List;
import java.util.Queue;

/**
 * Fake implementation of a {@link RequestManager}. This acts a Queue of responses which will be
 * sent through the {@link ProtocolAdapter} to create the {@code List<StreamDataOperation>} which is
 * then returned as a {@link Result} to the {@link Consumer}.
 */
public class FakeRequestManager implements RequestManager {
  private final ProtocolAdapter protocolAdapter;
  private final Queue<Response> responses = new ArrayDeque<>();

  public FakeRequestManager(ProtocolAdapter protocolAdapter) {
    this.protocolAdapter = protocolAdapter;
  }

  /** Adds a Response to the Queue. */
  public void queueResponse(Response response) {
    responses.add(response);
  }

  @Override
  public void loadMore(
      StreamToken streamToken, Consumer<Result<List<StreamDataOperation>>> consumer) {
    Response response = responses.remove();
    Result<List<StreamDataOperation>> result = protocolAdapter.createModel(response);
    Result<List<StreamDataOperation>> contextResult;
    if (result.isSuccessful()) {
      contextResult = Result.success(result.getValue());
    } else {
      contextResult = Result.failure();
    }
    consumer.accept(contextResult);
  }

  @Override
  public void triggerRefresh(
      RequestReason reason, Consumer<Result<List<StreamDataOperation>>> consumer) {
    Response response = responses.remove();
    Result<List<StreamDataOperation>> result = protocolAdapter.createModel(response);
    Result<List<StreamDataOperation>> contextResult;
    if (result.isSuccessful()) {
      contextResult = Result.success(result.getValue());
    } else {
      contextResult = Result.failure();
    }
    consumer.accept(contextResult);
  }
}
