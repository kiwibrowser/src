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

import android.support.v4.util.Pools.SimplePool;

/** A very simple, single pool version of a {@link RecyclerPool} */
class SingleKeyRecyclerPool<A extends ElementAdapter<?, ?>> implements RecyclerPool<A> {
  private static final String KEY_ERROR_MESSAGE = "Given key %s does not match singleton key %s";

  private final RecyclerKey singletonKey;
  private final int capacity;

  private SimplePool<A> pool;

  SingleKeyRecyclerPool(RecyclerKey key, int capacity) {
    singletonKey = key;
    this.capacity = capacity;
    pool = new SimplePool<>(capacity);
  }

  /*@Nullable*/
  @Override
  public A get(RecyclerKey key) {
    if (!singletonKey.equals(key)) {
      throw new IllegalArgumentException(String.format(KEY_ERROR_MESSAGE, key, singletonKey));
    }
    return pool.acquire();
  }

  @Override
  public void put(RecyclerKey key, A adapter) {
    if (key == null) {
      throw new NullPointerException(String.format("null key for %s", adapter));
    }
    if (!singletonKey.equals(key)) {
      throw new IllegalArgumentException(String.format(KEY_ERROR_MESSAGE, key, singletonKey));
    }
    pool.release(adapter);
  }

  @Override
  public void clear() {
    pool = new SimplePool<>(capacity);
  }
}
