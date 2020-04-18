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

package com.google.android.libraries.feed.feedsessionmanager.internal;

import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import java.util.HashMap;
import java.util.Map;

/**
 * In order to support optimistic writes, the cache must cache the payloads for the full mutation.
 * The cache supports a mutation lifecycle methods to start and finish the mutation. The cache will
 * hold onto the mutation payloads, until the next mutation is started. We do this allow multiple
 * ModelProviders sessions to access the mutation payloads after the initial mutation is processed.
 * This cache assumes that we update all ModelProviders within the life of a single mutation. This
 * is the current behavior of FeedModelProvider.
 */
public class ContentCache implements Dumpable {
  private static final String TAG = "ContentCache";

  private final Map<String, StreamPayload> mutationCache;

  private int lookupCount = 0;
  private int hitCount = 0;
  private int maxMutationCacheSize = 0;
  private int mutationsCount = 0;

  public ContentCache() {
    mutationCache = new HashMap<>();
  }

  /**
   * Called when the {@link com.google.android.libraries.feed.feedsessionmanager.FeedSessionManager}
   * commits a new mutation. Everything added to the cache must be retained until {@link
   * #finishMutation()} is called.
   */
  public void startMutation() {
    mutationsCount++;
    mutationCache.clear();
  }

  /**
   * Called when the {@link com.google.android.libraries.feed.feedsessionmanager.FeedSessionManager}
   * has finished the mutation. At this point it would be safe to clear the cache.
   */
  public void finishMutation() {
    if (mutationCache.size() > maxMutationCacheSize) {
      // TODO: Should we move the mutationCache.clear here?
      maxMutationCacheSize = mutationCache.size();
    }
  }

  /** Return the {@link StreamPayload} or {@code null} if it is not found in the cache. */
  /*@Nullable*/
  public StreamPayload get(String key) {
    StreamPayload value = mutationCache.get(key);
    lookupCount++;
    if (value != null) {
      hitCount++;
    } else {
      // This is expected on startup.
      Logger.d(TAG, "Mutation Cache didn't contain item %s", key);
    }
    return value;
  }

  /** Add a new value to the cache, returning the previous version or {@code null}. */
  /*@Nullable*/
  public StreamPayload put(String key, StreamPayload payload) {
    return mutationCache.put(key, payload);
  }

  /** Returns the current number of items in the cache. */
  public int size() {
    return mutationCache.size();
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    dumper.forKey("mutationCacheSize").value(mutationCache.size());
    dumper.forKey("mutationsCount").value(mutationsCount).compactPrevious();
    dumper.forKey("maxMutationCacheSize").value(maxMutationCacheSize).compactPrevious();
    dumper.forKey("lookupCount").value(lookupCount);
    dumper.forKey("hits").value(hitCount).compactPrevious();
    dumper.forKey("misses").value(lookupCount - hitCount).compactPrevious();
  }
}
