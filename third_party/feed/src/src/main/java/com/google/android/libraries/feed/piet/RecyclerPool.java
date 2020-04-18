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

package com.google.android.libraries.feed.piet;


/**
 * Interface defining a simple Pool of Adapters.
 *
 * @param <A> The adapter being managed by the {@link RecyclerPool}
 */
interface RecyclerPool<A extends ElementAdapter<?, ?>> {

  /**
   * Return an {@link ElementAdapter} matching the {@link RecyclerKey} or null if one isn't found.
   */
  /*@Nullable*/
  A get(RecyclerKey key);

  /** Put a {@link ElementAdapter} with a {@link RecyclerKey} into the pool. */
  void put(RecyclerKey key, A adapter);

  /** Clear everything out of the recycler pool. */
  void clear();
}
