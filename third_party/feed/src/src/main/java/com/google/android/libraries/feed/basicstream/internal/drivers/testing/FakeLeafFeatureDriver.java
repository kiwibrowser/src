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

package com.google.android.libraries.feed.basicstream.internal.drivers.testing;

import com.google.android.libraries.feed.basicstream.internal.drivers.LeafFeatureDriver;
import com.google.android.libraries.feed.basicstream.internal.viewholders.FeedViewHolder;
import com.google.android.libraries.feed.basicstream.internal.viewholders.ViewHolderType;

/** Fake for {@link LeafFeatureDriver}. */
public class FakeLeafFeatureDriver extends LeafFeatureDriver {

  private final int itemViewType;

  private FakeLeafFeatureDriver(int itemViewType) {
    this.itemViewType = itemViewType;
  }

  @Override
  public void bind(FeedViewHolder viewHolder) {}

  @Override
  public void unbind() {}

  @Override
  @ViewHolderType
  public int getItemViewType() {
    return itemViewType;
  }

  @Override
  public long itemId() {
    return hashCode();
  }

  @Override
  public LeafFeatureDriver getLeafFeatureDriver() {
    return this;
  }

  public static class Builder {
    @ViewHolderType private int itemViewType = ViewHolderType.TYPE_CARD;

    public Builder setItemViewType(@ViewHolderType int viewType) {
      itemViewType = viewType;
      return this;
    }

    public LeafFeatureDriver build() {
      return new FakeLeafFeatureDriver(itemViewType);
    }
  }
}
