/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.ipc.invalidation.ticl;

import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.util.TypedUtil;

import java.util.HashMap;
import java.util.Map;

/**
 * An ack "cache" that allows the TICL to avoid unnecessary delivery of a
 * known-version invalidation when the client has aleady acked a known-version,
 * restarted invalidation with the same or a greater version number.
 * <p>
 * This helps invalidation clients avoid unnecessary syncs against their backend
 * when invalidations for an object are redelivered or reordered, as can occur
 * frequently during a PCR or (to a lesser degree) as a result of internal
 * failures and channel flakiness.
 * <p>
 * This optimization is especially useful for applications that want to use
 * the TI Pubsub API to deliver invalidations, because version numbers are not
 * a concept in the API itself. While the client could include version numbers
 * in the payload, truncation messages do not include a payload.
 * <p>
 * The  cache invalidation API does expose version numbers, so client
 * applications could implement the same logic themselves, but many
 * do not, so it is a useful convenience to implement this for them in the TICL.
 * <p>
 * Note this class currently only records acks for restarted, known-version
 * invalidations. While we might add ack tracking for continous invalidations at
 * some time in the future, tracking continuous invalidations has less of a
 * payoff than tracking restarted invalidations, because such an ack does not
 * implicitly ack earlier invalidations for that object, and greater complexity,
 * because of the potentially unbounded number of acks that need to be tracked
 * for each object.
 */
class AckCache {

  /**
   * A map from object id to the (long) version number of the highest
   * <em>restarted, known version</em> invalidation for that object that has
   * been acked by the client.
   */
  private Map<ObjectIdP, Long> highestAckedVersionMap = new HashMap<ObjectIdP, Long>();

  /** Records the fact that the client has acknowledged the given invalidation. */
  void recordAck(InvalidationP inv) {
    if (!inv.getIsTrickleRestart() || !inv.getIsKnownVersion()) {
      return;
    }

    // If the invalidation version is newer than the highest acked version in the
    // map, then update the map.
    ObjectIdP objectId = inv.getObjectId();
    long version = inv.getVersion();
    if (version > getHighestAckedVersion(objectId)) {
      highestAckedVersionMap.put(objectId, version);
    }
  }

  /**
   * Returns true if the client has already acked a restarted invalidation with
   * a version number greater than or equal to that in {@code inv} and the same
   * object id, and {@code inv} is a known version invalidation. Unknown version
   * invalidations are never considered already acked.
   */
  boolean isAcked(InvalidationP inv) {
    return inv.getIsKnownVersion()
        && this.getHighestAckedVersion(inv.getObjectId()) >= inv.getVersion();
  }


  /**
   * Returns the highest acked version for the object id with the given key, or
   * -1 if no versions have been acked.
   */
  private long getHighestAckedVersion(ObjectIdP objectId) {
    Long version = TypedUtil.mapGet(highestAckedVersionMap, objectId);
    return (version != null) ? version : -1L;
  }
}
