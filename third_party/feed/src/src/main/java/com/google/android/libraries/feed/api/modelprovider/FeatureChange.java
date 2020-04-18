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

import java.util.List;

/**
 * Class defining the specific changes made to feature nodes within the model. This is passed to the
 * observer to handle model changes. There are a few types of changes described here:
 *
 * <ol>
 *   <li>if {@link #isFeatureChanged()} returns {@code true}, then the ModelFeature was changed
 *   <li>{@link #getChildChanges()} returns a list of features either, appended, prepended, or
 *       removed from a parent
 * </ol>
 */
public interface FeatureChange {

  /** Returns the contentId of the ModelFeature which was changed. */
  String getContentId();

  /** Returns {@code true} if the ModelFeature changed. */
  boolean isFeatureChanged();

  /** Returns the ModelFeature that was changed. */
  ModelFeature getModelFeature();

  /** Returns the structural changes to the ModelFeature. */
  ChildChanges getChildChanges();

  /** Class describing changes to the children. */
  interface ChildChanges {
    /**
     * Returns a List of the children added to this ModelFeature. These children are in the same
     * order they would be displayed in the stream.
     */
    List<ModelChild> getAppendedChildren();

    /** Returns a List of the children removed from this ModelFeature. */
    List<ModelChild> getRemovedChildren();
  }
}
