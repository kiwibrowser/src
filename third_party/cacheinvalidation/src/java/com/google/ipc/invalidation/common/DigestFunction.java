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

/**
 * Interface specifying a function to compute digests.
 *
 */
public interface DigestFunction {
  /** Clears the digest state. */
  void reset();

  /** Adds {@code data} to the digest being computed. */
  void update(byte[] data);

  /**
   * Returns the digest of the data added by {@link #update}. After this call has been made,
   * reset must be called before {@link #update} and {@link #getDigest} can be called.
   */
  byte[] getDigest();
}
