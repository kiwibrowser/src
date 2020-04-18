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


/**
 * This represents a Cursor through the children of a {@link ModelFeature}. A Cursor will provide
 * forward only access to the children of the container. When a cursor is created by calling {@link
 * ModelFeature#getCursor()}, it is positioned at the first child.
 */
public interface ModelCursor {
  /** Returns the next {@link ModelChild} in the cursor or {@code null} if at end. */
  /*@Nullable*/
  ModelChild getNextItem();

  /** Returns {@literal true} if the cursor is at the end. */
  boolean isAtEnd();
}
