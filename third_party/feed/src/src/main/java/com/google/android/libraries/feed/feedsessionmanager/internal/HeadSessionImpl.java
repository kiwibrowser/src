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

import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.store.SessionMutation;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import com.google.search.now.feed.client.StreamDataProto.StreamSession;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure;
import com.google.search.now.feed.client.StreamDataProto.StreamStructure.Operation;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Implementation of {@link Session} for $HEAD. This class doesn't support a ModelProvider. The
 * $HEAD session does not support optimistic writes because we may create a new session between when
 * the response is received and when task updating the head session runs.
 */
public class HeadSessionImpl implements Session, Dumpable {
  private static final String TAG = "HeadSessionImpl";

  private StreamSession streamSession;
  private final Store store;
  private final TimingUtils timingUtils;

  @VisibleForTesting final Set<String> contentInSession = new HashSet<>();

  // operation counts for the dumper
  private int updateCount = 0;
  private int storeMutationFailures = 0;

  HeadSessionImpl(Store store, TimingUtils timingUtils) {
    this.streamSession = store.getHeadSession();
    this.store = store;
    this.timingUtils = timingUtils;
  }

  /** Initialize head from the stored stream structures. */
  public void initializeSession(List<StreamStructure> streamStructures) {
    for (StreamStructure streamStructure : streamStructures) {
      contentInSession.add(streamStructure.getContentId());
    }
  }

  @Override
  public void updateSession(
      List<StreamStructure> streamStructures, /*@Nullable*/ StreamToken mutationSourceToken) {
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    updateCount++;

    if (mutationSourceToken != null) {
      String contentId = mutationSourceToken.getContentId();
      if (contentId != null && !contentInSession.contains(contentId)) {
        // Make sure that mutationSourceToken is a token in this session, if not, we don't update
        // the session.
        timeTracker.stop(
            "updateSessionIgnored", streamSession.getStreamToken(), "Token Not Found", contentId);
        Logger.i(TAG, "Token %s not found in session, ignoring update", contentId);
        return;
      }
    }

    int updateCnt = 0;
    int addFeatureCnt = 0;
    boolean cleared = false;
    SessionMutation sessionMutation = store.editSession(streamSession);
    for (StreamStructure streamStructure : streamStructures) {
      if (streamStructure.getOperation() == Operation.UPDATE_OR_APPEND) {
        String contentKey = streamStructure.getContentId();
        if (contentInSession.contains(contentKey)) {
          // this is an update operation so we can ignore it
          updateCnt++;
          continue;
        }

        sessionMutation.add(streamStructure);
        contentInSession.add(contentKey);
        addFeatureCnt++;
        continue;
      } else if (streamStructure.getOperation() == Operation.REMOVE) {
        String contentKey = streamStructure.getContentId();
        Logger.i(TAG, "Removing Item %s from $HEAD", contentKey);
        if (contentInSession.contains(contentKey)) {
          sessionMutation.add(streamStructure);
          contentInSession.remove(contentKey);
        } else {
          Logger.w(TAG, "Remove operation content not found in $HEAD");
        }
        continue;
      } else if (streamStructure.getOperation() == Operation.CLEAR_ALL) {
        contentInSession.clear();
        cleared = true;
        continue;
      }
      Logger.e(TAG, "Unknown operation, ignoring: %s", streamStructure.getOperation());
    }
    boolean success = sessionMutation.commit();
    if (success) {
      timeTracker.stop(
          "updateSession",
          streamSession.getStreamToken(),
          "cleared",
          cleared,
          "features",
          addFeatureCnt,
          "updates",
          updateCnt);
    } else {
      timeTracker.stop("updateSessionFailure", streamSession.getStreamToken());
      storeMutationFailures++;
      // TODO: What do we do if this fails?
      Logger.e(TAG, "$HEAD session mutation failed");
    }
  }

  @Override
  public StreamSession getStreamSession() {
    return streamSession;
  }

  @Override
  public void updateAccessTime(long time) {
    streamSession = streamSession.toBuilder().setLastAccessed(time).build();
  }

  @Override
  /*@Nullable*/
  public ModelProvider getModelProvider() {
    return null;
  }

  @Override
  public Collection<String> getContentInSession() {
    return Collections.unmodifiableCollection(contentInSession);
  }

  public boolean isHeadEmpty() {
    return contentInSession.isEmpty();
  }

  // This is called only in tests to acess the content in the session.
  public Set<String> getContentInSessionForTest() {
    return new HashSet<>(contentInSession);
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    dumper.forKey("sessionName").value(streamSession.getStreamToken());
    dumper.forKey("accessTime").value(new Date(streamSession.getLastAccessed())).compactPrevious();
    dumper.forKey("contentInSession").value(contentInSession.size());
    dumper.forKey("updateCount").value(updateCount).compactPrevious();
    dumper.forKey("storeMutationFailures").value(storeMutationFailures).compactPrevious();
  }
}
