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

package com.google.android.libraries.feed.api.sessionmanager;

import com.google.android.libraries.feed.api.common.MutationContext;
import com.google.android.libraries.feed.api.common.PayloadWithId;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.search.now.feed.client.StreamDataProto.StreamDataOperation;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamSharedState;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import java.util.List;

/** The SessionManager is responsible for creating and updating sessions within Jardin. */
public interface SessionManager {

  /**
   * This is called by the UI to get a new session. The life of the session is controlled by the
   * Session manager. The SessionManager maintains HEAD$ which represents the most current state of
   * the stream. It will also decide which changes should be made to existing sessions and the life
   * time of existing sessions.
   */
  void getNewSession(ModelProvider modelProvider);

  /**
   * Create a new Session attached to the ModelProvider for an existing sessionId. This will
   * invalidate any existing ModelProvider instances attached to the session.
   *
   * @return false if the existing session is not found.
   */
  boolean getExistingSession(String sessionToken, ModelProvider modelProvider);

  /**
   * This method will invalidate head. All {@link #getNewSession(ModelProvider)} will block until a
   * new a response updates the SessionManager providing the new head.
   */
  void invalidateHead();

  /**
   * Method which causes a request to be created for the next page of content based upon the
   * continuation token. This could be called from multiple Model Providers.
   */
  void handleToken(StreamSession session, StreamToken streamToken);

  /** Method which causes a refresh */
  void triggerRefresh(/*@Nullable*/ StreamSession streamSession);

  /**
   * Returns a List of {@link StreamPayload} for each of the keys. This operation may perform disk
   * reads, therefore the payloads are returned through a consumer. The returned values will contain
   * both the payload and the content id of the payload.
   */
  void getStreamFeatures(List<String> contentIds, Consumer<List<PayloadWithId>> consumer);

  /**
   * Return the shared state. This operation will be fast, so it can be called on the UI Thread.
   * This method will return {@code null} if the shared state is not found.
   */
  /*@Nullable*/
  StreamSharedState getSharedState(ContentId contentId);

  /**
   * Return a {@link Consumer} which is able to handle an update to the SessionManager. An update
   * consists of a List of {@link StreamDataOperation} objects. The {@link MutationContext} captures
   * the context which is initiating the update operation.
   */
  Consumer<Result<List<StreamDataOperation>>> getUpdateConsumer(MutationContext mutationContext);

  /**
   * This will run a Task on the SessionManager's Executor.
   *
   * <p>TODO: Remove this when TaskQueue is moved outside of FeedModelProvider
   */
  void runTask(String task, Runnable runnable);
}
