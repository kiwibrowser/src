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

/**
 * Abstract Interface defining a Child of a feature which may be returned by a {@link ModelCursor}.
 * A ModelChild must be bound to either a feature or token.
 */
public interface ModelChild {

  /** Define the concrete type of the children. */
  @IntDef({Type.UNBOUND, Type.FEATURE, Type.TOKEN})
  @interface Type {
    // A feature that isn't yet bound to a specific data type
    // This should never be exposed outside of the model.
    int UNBOUND = 0;

    // A stream feature child.
    int FEATURE = 1;

    // The child is a continuation token.  This means the cursor is at end
    // and a request has been made to create a new cursor with additional
    // children.
    int TOKEN = 2;
  }

  /** Returns the type of the child. */
  @Type
  int getType();

  /** Returns the ContentId of the child, all children will have a Content ID. */
  String getContentId();

  /** Returns true there is a parent to this child; false if this is a root. */
  boolean hasParentId();

  /**
   * Returns the parent contentId of the child, this may return {@code Null} if the child is a root.
   */
  /*@Nullable*/
  String getParentId();

  /**
   * Returns a the {@link ModelFeature}. This will throw an {@link IllegalStateException} if the
   * {@link Type} is not equal to {@code FEATURE}.
   */
  ModelFeature getModelFeature();

  /**
   * Returns the {@link ModelToken}. This will throw an {@link IllegalStateException} if the {@link
   * Type} is not equal to {@code TOKEN}.
   */
  ModelToken getModelToken();
}
