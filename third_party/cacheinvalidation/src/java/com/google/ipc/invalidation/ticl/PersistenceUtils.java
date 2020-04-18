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

import com.google.ipc.invalidation.common.DigestFunction;
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.ticl.proto.Client.PersistentStateBlob;
import com.google.ipc.invalidation.ticl.proto.Client.PersistentTiclState;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.ProtoWrapper.ValidationException;
import com.google.ipc.invalidation.util.TypedUtil;

/**
 * Utility methods for handling the Ticl persistent state.
 *
 */
public class PersistenceUtils {

  /** Serializes a Ticl state blob. */
  public static byte[] serializeState(
      PersistentTiclState state, DigestFunction digestFn) {
    Bytes mac = generateMac(state, digestFn);
    return PersistentStateBlob.create(state, mac).toByteArray();
  }

  /**
   * Deserializes a Ticl state blob. Returns either the parsed state or {@code null}
   * if it could not be parsed.
   */
  public static PersistentTiclState deserializeState(Logger logger, byte[] stateBlobBytes,
      DigestFunction digestFn) {
    PersistentStateBlob stateBlob;
    try {
      // Try parsing the envelope protocol buffer.
      stateBlob = PersistentStateBlob.parseFrom(stateBlobBytes);
    } catch (ValidationException exception) {
      logger.severe("Failed deserializing Ticl state: %s", exception.getMessage());
      return null;
    }

    // Check the mac in the envelope against the recomputed mac from the state.
    PersistentTiclState ticlState = stateBlob.getTiclState();
    Bytes mac = generateMac(ticlState, digestFn);
    if (!TypedUtil.<Bytes>equals(mac, stateBlob.getAuthenticationCode())) {
      logger.warning("Ticl state failed MAC check: computed %s vs %s", mac,
          stateBlob.getAuthenticationCode());
      return null;
    }
    return ticlState;
  }

  /** Returns a message authentication code over {@code state}. */
  private static Bytes generateMac(PersistentTiclState state, DigestFunction digestFn) {
    digestFn.reset();
    digestFn.update(state.toByteArray());
    return new Bytes(digestFn.getDigest());
  }

  private PersistenceUtils() {
    // Prevent instantiation.
  }
}
