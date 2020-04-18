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

import com.google.ipc.invalidation.ticl.proto.Client.ExponentialBackoffState;
import com.google.ipc.invalidation.util.ExponentialBackoffDelayGenerator;
import com.google.ipc.invalidation.util.Marshallable;

import java.util.Random;

/**
 * A subclass of {@link ExponentialBackoffDelayGenerator} that supports (un)marshalling to and from
 * protocol buffers.
 *
 */
public class TiclExponentialBackoffDelayGenerator
    extends ExponentialBackoffDelayGenerator implements Marshallable<ExponentialBackoffState> {

  /**
   * Creates an exponential backoff delay generator. Parameters  are as in
   * {@link ExponentialBackoffDelayGenerator#ExponentialBackoffDelayGenerator(Random, int, int)}.
   */
  public TiclExponentialBackoffDelayGenerator(Random random, int initialMaxDelay,
      int maxExponentialFactor) {
    super(random, initialMaxDelay, maxExponentialFactor);
  }

  /**
   * Restores a generator from {@code marshalledState}. Other parameters are as in
   * {@link ExponentialBackoffDelayGenerator#ExponentialBackoffDelayGenerator(Random, int, int)}.
   *
   * @param marshalledState marshalled state from which to restore.
   */
  public TiclExponentialBackoffDelayGenerator(Random random, int initialMaxDelay,
      int maxExponentialFactor, ExponentialBackoffState marshalledState) {
    super(random, initialMaxDelay, maxExponentialFactor, marshalledState.getCurrentMaxDelay(),
        marshalledState.getInRetryMode());
  }

  @Override
  public ExponentialBackoffState marshal() {
    return ExponentialBackoffState.create(getCurrentMaxDelay(), getInRetryMode());
  }
}
