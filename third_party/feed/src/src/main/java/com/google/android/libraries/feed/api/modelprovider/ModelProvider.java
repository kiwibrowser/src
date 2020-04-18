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

package com.google.android.libraries.feed.api.modelprovider;

import android.support.annotation.IntDef;
import com.google.search.now.feed.client.StreamDataProto.StreamSharedState;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;

/**
 * A ModelProvider provides access to the Stream Model for the UI layer. It is populated by the
 * Session Manager. The ModelProvider is backed by a Session instance, there is a one-to-one
 * relationship between the ModelProvider and Session implementation. The Stream Library uses the
 * model to build the UI displayed to the user.
 */
public interface ModelProvider {
  /** Returns a Mutator used to change the model. */
  ModelMutation edit();

  /**
   * This is called to invalidate the model. The SessionManager calls this to free all resources
   * held by this ModelProvider instance.
   */
  void invalidate();

  /**
   * Returns a {@code ModelFeature} which represents the root of the stream tree. Returns {@code
   * null} if the stream is empty.
   */
  /*@Nullable*/
  ModelFeature getRootFeature();

  /** Return a ModelChild for a String ContentId */
  /*@Nullable*/
  ModelChild getModelChild(String contentId);

  /**
   * Returns a {@code StreamSharedState} containing shared state such as the Piet shard state.
   * Returns {@code null} if the shared state is not found.
   */
  /*@Nullable*/
  StreamSharedState getSharedState(ContentId contentId);

  /**
   * Handle the processing of a {@code ModelToken}. For example start a request for the next page of
   * content. The results of handling the token will be available through Observers on the
   * ModelToken.
   */
  void handleToken(ModelToken modelToken);

  /**
   * Allow the stream to force a refresh. This will result in the current model being invalidated if
   * the requested refresh is successful.
   */
  void triggerRefresh();

  /** Defines the Lifecycle of the ModelProvider */
  @IntDef({State.INITIALIZING, State.READY, State.INVALIDATED})
  @interface State {
    /**
     * State of the Model Provider before it has been fully initialized. The model shouldn't be
     * accessed before it enters the {@code READY} state. You should register an Observer to receive
     * an event when the model is ready.
     */
    int INITIALIZING = 0;
    /** State of the Model Provider when it is ready for normal use. */
    int READY = 1;
    /**
     * State of the Model Provider when it has been invalidated. In this mode, the Model is no
     * longer valid and methods will fail.
     */
    int INVALIDATED = 2;
  }

  /** Returns the current state of the ModelProvider */
  @State
  int getCurrentState();

  /** A String which represents the session bound to the ModelProvider. */
  /*@Nullable*/
  String getSessionToken();

  /**
   * Register a {@link ModelProviderObserver} for changes on this container. The types of changes
   * would include adding or removing children or updates to the metadata payload.
   */
  void registerObserver(ModelProviderObserver observer);

  /** Remove a registered observer */
  void unregisterObserver(ModelProviderObserver observer);
}
