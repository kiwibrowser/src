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

package com.google.android.libraries.feed.api.protocoladapter;

import com.google.android.libraries.feed.common.Result;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import com.google.search.now.wire.feed.DataOperationProto.DataOperation;
import com.google.search.now.wire.feed.ResponseProto.Response;
import java.util.List;

/** Converts the wire protocol (protos sent from the server) into an internal representation. */
public interface ProtocolAdapter {
  /**
   * Create the internal protocol from a wire protocol response definition. The wire protocol is
   * turned into a List of {@link StreamDataOperation} which are sent to the SessionManager.
   */
  Result<List<StreamDataOperation>> createModel(Response response);

  /**
   * Create {@link StreamDataOperation}s from the internal protocol for the wire protocol
   * DataOperations.
   */
  Result<List<StreamDataOperation>> createOperations(List<DataOperation> dataOperations);

  /**
   * Convert a wire protocol ContentId into the {@code String} version. Inverse of {@link
   * #getWireContentId(String)}
   */
  String getStreamContentId(ContentId contentId);

  /**
   * Convert a string ContentId into the wire protocol version. Inverse of {@link
   * #getStreamContentId(ContentId)}. Note that due to default proto values, if no ID was set in
   * {@link #getStreamContentId(ContentId)}, this method will set the ID to 0.
   */
  Result<ContentId> getWireContentId(String contentId);
}
