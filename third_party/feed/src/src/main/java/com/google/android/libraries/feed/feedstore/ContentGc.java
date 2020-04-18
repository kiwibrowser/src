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

package com.google.android.libraries.feed.feedstore;

import static com.google.android.libraries.feed.feedstore.FeedStore.SEMANTIC_PROPERTIES_PREFIX;
import static com.google.android.libraries.feed.feedstore.FeedStore.SHARED_STATE_PREFIX;

import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.concurrent.SimpleSettableFuture;
import com.google.android.libraries.feed.common.functional.Supplier;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.android.libraries.feed.host.storage.ContentMutation;
import com.google.android.libraries.feed.host.storage.ContentStorage;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutionException;

/** Storage Content Garbage Collector. */
class ContentGc {
  private static final String TAG = "ContentGc";

  private final Supplier<Set<String>> accessibleContentSupplier;
  private final Set<String> reservedContentIds;
  private final ContentStorage contentStorage;
  private final MainThreadRunner mainThreadRunner;
  private final TimingUtils timingUtils;

  ContentGc(
      Supplier<Set<String>> accessibleContentSupplier,
      Set<String> reservedContentIds,
      ContentStorage contentStorage,
      MainThreadRunner mainThreadRunner,
      TimingUtils timingUtils) {
    this.accessibleContentSupplier = accessibleContentSupplier;
    this.reservedContentIds = reservedContentIds;
    this.contentStorage = contentStorage;
    this.mainThreadRunner = mainThreadRunner;
    this.timingUtils = timingUtils;
  }

  void gc() {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> population = getPopulation();
    // remove the items in the population that are accessible or reserved
    population.removeAll(getAccessible());
    population.removeAll(reservedContentIds);
    filterPrefixed(population);

    // Population now contains only un-accessible items
    removeUnAccessible(population);
    tracker.stop("", "ContentGc", "contentRemoved", population.size());
  }

  private void removeUnAccessible(Set<String> unAccessible) {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    ContentMutation.Builder mutationBuilder = new ContentMutation.Builder();
    for (String key : unAccessible) {
      mutationBuilder.delete(key);
    }
    SimpleSettableFuture<CommitResult> contentFuture = new SimpleSettableFuture<>();
    try {
      mainThreadRunner.execute(
          TAG, () -> contentStorage.commit(mutationBuilder.build(), contentFuture::put));
      CommitResult result = contentFuture.get();
      if (result == CommitResult.FAILURE) {
        // TODO: How do we handle the errors?
        Logger.e(TAG, "Content Modification Failed");
      }
      tracker.stop("", "removeUnAccessible", "mutations", unAccessible.size());
    } catch (InterruptedException | ExecutionException e) {
      // TODO: ???
      throw new IllegalStateException("Couldn't read stream structures", e);
    }
  }

  private void filterPrefixed(Set<String> population) {
    int size = population.size();
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Iterator<String> i = population.iterator();
    while (i.hasNext()) {
      String key = i.next();
      if (key.startsWith(SEMANTIC_PROPERTIES_PREFIX) || key.startsWith(SHARED_STATE_PREFIX)) {
        i.remove();
      }
    }
    tracker.stop("", "filterPrefixed", population.size() - size);
  }

  private Set<String> getAccessible() {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> accessibleContent = accessibleContentSupplier.get();
    tracker.stop("", "getAccessible", "accessableContent", accessibleContent.size());
    return accessibleContent;
  }

  private Set<String> getPopulation() {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> population = new HashSet<>();
    SimpleSettableFuture<Result<Map<String, byte[]>>> contentFuture = new SimpleSettableFuture<>();
    try {
      mainThreadRunner.execute(TAG, () -> contentStorage.getAll("", contentFuture::put));
      Result<Map<String, byte[]>> result = contentFuture.get();
      if (result.isSuccessful()) {
        population.addAll(result.getValue().keySet());
      } else {
        Logger.e(TAG, "Unable to get all content, getAll failed");
      }
      tracker.stop("", "getPopulation", "contentPopulation", population.size());
      return population;
    } catch (InterruptedException | ExecutionException e) {
      // TODO: ???
      throw new IllegalStateException("Couldn't read stream structures", e);
    }
  }
}
