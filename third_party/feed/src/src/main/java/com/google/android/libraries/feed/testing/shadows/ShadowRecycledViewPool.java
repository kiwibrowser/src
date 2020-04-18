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

package com.google.android.libraries.feed.testing.shadows;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

/**
 * Shadow for {@link RecyclerView.RecycledViewPool} which is able to give information on when the
 * pool has been cleared.
 *
 * <p>{@link ViewHolder#getItemViewType()} is used as the keys for pools; however, this field is
 * final. That means a special adapters needs to be created in order to handle all this. This shadow
 * helps so all this infrastructure isn't needed.
 */
@Implements(RecyclerView.RecycledViewPool.class)
public class ShadowRecycledViewPool {

  private int clearCount;

  @Implementation
  public void clear() {
    clearCount++;
  }

  public int getClearCallCount() {
    return clearCount;
  }
}
