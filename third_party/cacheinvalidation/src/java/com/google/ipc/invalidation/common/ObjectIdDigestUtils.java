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

package com.google.ipc.invalidation.common;

import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.Preconditions;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * Digest-related utilities for object ids.
 *
 */
public class ObjectIdDigestUtils {
  /**
   * Implementation of {@link DigestFunction} using SHA-1.
   */
  public static class Sha1DigestFunction
      implements DigestFunction {
    /** Digest implementation. */
    private MessageDigest sha1;

    /**
     * Whether the {@link #reset()} method needs to be called. This is set to true
     * when {@link #getDigest()} is called and aims to prevent subtle bugs caused by
     * failing to reset the object before computing a new digest.
     */
    private boolean resetNeeded = false;

    public Sha1DigestFunction() {
      try {
      this.sha1 = MessageDigest.getInstance("SHA-1");
      } catch (NoSuchAlgorithmException exception) {
        throw new RuntimeException(exception);
      }
    }

    @Override
    public void reset() {
      resetNeeded = false;
      sha1.reset();
    }

    @Override
    public void update(byte[] data) {
      Preconditions.checkState(!resetNeeded);
      sha1.update(data);
    }

    @Override
    public byte[] getDigest() {
      Preconditions.checkState(!resetNeeded);
      resetNeeded = true;
      return sha1.digest();
    }
  }

  /**
   * Returns the digest of {@code objectIdDigests} using {@code digestFn}.
   * <p>
   * REQUIRES: {@code objectIdDigests} iterate in sorted order.
   */
  public static Bytes getDigest(Iterable<Bytes> objectIdDigests, DigestFunction digestFn) {
    digestFn.reset();
    for (Bytes objectIdDigest : objectIdDigests) {
      digestFn.update(objectIdDigest.getByteArray());
    }
    return new Bytes(digestFn.getDigest());
  }

  /**
   * Returns the digest for the object id with source {@code objectSource} and name
   * {@code objectName} using {@code digestFn}.
   */
  public static Bytes getDigest(int objectSource, byte[] objectName, DigestFunction digestFn) {
    digestFn.reset();
    ByteBuffer buffer = ByteBuffer.allocate(Integer.SIZE / 8).order(ByteOrder.LITTLE_ENDIAN);
    buffer.putInt(objectSource);

    // Little endian number for type followed by bytes.
    digestFn.update(buffer.array());
    digestFn.update(objectName);
    return new Bytes(digestFn.getDigest());
  }
}
