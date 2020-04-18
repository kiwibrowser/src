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

package com.google.android.libraries.feed.basicstream.internal.drivers;

import com.google.android.libraries.feed.basicstream.internal.viewholders.FeedViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.ViewHolderType;

/** A {@link FeatureDriver} that can bind to a {@link FeedViewHolder}. */
public abstract class LeafFeatureDriver implements FeatureDriver {

  @Override
  public LeafFeatureDriver getLeafFeatureDriver() {
    return this;
  }

  /** Bind to the given {@link FeedViewHolder}. */
  public abstract void bind(FeedViewHolder viewHolder);

  /**
   * Returns a {@link ViewHolderType} that corresponds to the type of {@link FeedViewHolder} that
   * can be bound to.
   */
  @ViewHolderType
  public abstract int getItemViewType();

  /** Returns an ID corresponding to this item, typically a hashcode. */
  public long itemId() {
    return hashCode();
  }

  /**
   * Called when the {@link
   * com.google.android.libraries.feed.basicstream.internal.viewholders.FeedViewHolder} is
   * offscreen/unbound.
   *
   * <p>Note: {@link LeafFeatureDriver} instances should do work they need to to unbind themselves
   * and their previously bound {@link FeedViewHolder}.
   */
  public abstract void unbind();
}
