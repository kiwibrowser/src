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

import com.google.ipc.invalidation.external.client.types.Invalidation;
import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.ticl.proto.CommonProtos;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.Preconditions;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Utilities to convert between {@link com.google.ipc.invalidation.util.ProtoWrapper ProtoWrapper}
 * wrappers and externally-exposed types in the Ticl.
 */
public class ProtoWrapperConverter {

  /**
   * Converts an object id protocol buffer {@code objectId} to the
   * corresponding external type and returns it.
   */
  public static ObjectId convertFromObjectIdProto(ObjectIdP objectIdProto) {
    Preconditions.checkNotNull(objectIdProto);
    return ObjectId.newInstance(objectIdProto.getSource(), objectIdProto.getName().getByteArray());
  }

  /**
   * Converts an object id {@code objectId} to the corresponding protocol buffer
   * and returns it.
   */
  public static ObjectIdP convertToObjectIdProto(ObjectId objectId) {
    Preconditions.checkNotNull(objectId);
    return ObjectIdP.create(objectId.getSource(), new Bytes(objectId.getName()));
  }

  /**
   * Returns a list of {@link ObjectIdP} by converting each element of {@code objectIds} to
   * an {@code ObjectIdP}.
   */
  public static Collection<ObjectIdP> convertToObjectIdProtoCollection(
      Iterable<ObjectId> objectIds) {
    int expectedSize = (objectIds instanceof Collection) ? ((Collection<?>) objectIds).size() : 1;
    ArrayList<ObjectIdP> objectIdPs = new ArrayList<ObjectIdP>(expectedSize);
    for (ObjectId objectId : objectIds) {
      objectIdPs.add(convertToObjectIdProto(objectId));
    }
    return objectIdPs;
  }

  /**
   * Returns a list of {@link ObjectId} by converting each element of {@code objectIdProtos} to
   * an {@code ObjectId}.
   */
  public static Collection<ObjectId> convertFromObjectIdProtoCollection(
      Collection<ObjectIdP> objectIdProtos) {
    ArrayList<ObjectId> objectIds = new ArrayList<ObjectId>(objectIdProtos.size());
    for (ObjectIdP objectIdProto : objectIdProtos) {
      objectIds.add(convertFromObjectIdProto(objectIdProto));
    }
    return objectIds;
  }

  /**
   * Converts an invalidation protocol buffer {@code invalidation} to the
   * corresponding external object and returns it
   */
  public static Invalidation convertFromInvalidationProto(InvalidationP invalidation) {
    Preconditions.checkNotNull(invalidation);
    ObjectId objectId = convertFromObjectIdProto(invalidation.getObjectId());

    // No bridge arrival time in invalidation.
    return Invalidation.newInstance(objectId, invalidation.getVersion(),
        invalidation.hasPayload() ? invalidation.getPayload().getByteArray() : null,
            invalidation.getIsTrickleRestart());
  }

  /**
   * Converts an invalidation {@code invalidation} to the corresponding protocol
   * buffer and returns it.
   */
  public static InvalidationP convertToInvalidationProto(Invalidation invalidation) {
    Preconditions.checkNotNull(invalidation);
    ObjectIdP objectId = convertToObjectIdProto(invalidation.getObjectId());

    // Invalidations clients do not know about trickle restarts. Every invalidation is allowed
    // to suppress earlier invalidations and acks implicitly acknowledge all previous
    // invalidations. Therefore the correct semanantics are provided by setting isTrickleRestart to
    // true.
    return CommonProtos.newInvalidationP(objectId, invalidation.getVersion(),
        invalidation.getIsTrickleRestartForInternalUse(), invalidation.getPayload());
  }

  private ProtoWrapperConverter() { // To prevent instantiation.
  }
}
