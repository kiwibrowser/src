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

import com.google.search.now.feed.client.StreamDataProto.StreamFeature;

/**
 * This represents a Feature model within the stream tree structure. This represents things such as
 * a Cluster, Carousel, Piet Content, etc. Features provide an {@link ModelCursor} for accessing the
 * children of the feature.
 */
public interface ModelFeature {

  /**
   * Returns the {@link StreamFeature} proto instance allowing for access of the metadata and
   * payload.
   */
  StreamFeature getStreamFeature();

  /**
   * An Cursor over the children of the feature. This Cursor is a one way iterator over the
   * children. If the feature does not contain children, an empty cursor will be returned.
   */
  ModelCursor getCursor();

  /**
   * Create a ModelCursor which advances in the defined direction (forward or reverse), it may also
   * start at a specific child. If the specified child is not found, this will return {@code null}.
   * If {@code startingChild} is {@code null}, the cursor starts at the start (beginning or end) of
   * the child list.
   */
  /*@Nullable*/
  ModelCursor getDirectionalCursor(boolean forward, /*@Nullable*/ String startingChild);

  /**
   * Register a {@link FeatureChangeObserver} for changes on this feature. The types of changes
   * would include adding or removing children or updates to the payload.
   */
  void registerObserver(FeatureChangeObserver observer);

  /** Remove a registered observer */
  void unregisterObserver(FeatureChangeObserver observer);
}
