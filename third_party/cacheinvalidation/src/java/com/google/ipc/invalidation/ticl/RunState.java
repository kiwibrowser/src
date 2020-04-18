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

import com.google.ipc.invalidation.ticl.proto.Client.RunStateP;
import com.google.ipc.invalidation.util.Marshallable;

/**
 * An abstraction that keeps track of whether the caller is started or stopped and only allows
 * the following transitions NOT_STARTED -> STARTED -> STOPPED. This class is thread-safe.
 *
 */
public class RunState implements Marshallable<RunStateP> {
  /** Current run state ({@link RunStateP}). */
  private Integer currentState;
  private final Object lock = new Object();

  /** Constructs a new instance in the {@code NOT_STARTED} state. */
  public RunState() {
    currentState = RunStateP.State.NOT_STARTED;
  }

  /** Constructs a new instance with the state given in {@code runState}. */
  RunState(RunStateP runState) {
    this.currentState = runState.getState();
  }

  /**
   * Marks the current state to be STARTED.
   * <p>
   * REQUIRES: Current state is NOT_STARTED.
   */
  public void start() {
    synchronized (lock) {
      if (currentState != RunStateP.State.NOT_STARTED) {
        throw new IllegalStateException("Cannot start: " + currentState);
      }
      currentState = RunStateP.State.STARTED;
    }
  }

  /**
   * Marks the current state to be STOPPED.
   * <p>
   * REQUIRES: Current state is STARTED.
   */
  public void stop() {
    synchronized (lock) {
      if (currentState != RunStateP.State.STARTED) {
        throw new IllegalStateException("Cannot stop: " + currentState);
      }
      currentState = RunStateP.State.STOPPED;
    }
  }

  /**
   * Returns true iff {@link #start} has been called on this but {@link #stop} has not been called.
   */
  public boolean isStarted() {
    synchronized (lock) {
      return currentState == RunStateP.State.STARTED;
    }
  }

  /** Returns true iff {@link #start} and {@link #stop} have been called on this object. */
  public boolean isStopped() {
    synchronized (lock) {
      return currentState == RunStateP.State.STOPPED;
    }
  }

  @Override
  public RunStateP marshal() {
    return RunStateP.create(currentState);
  }

  @Override
  public String toString() {
    return "<RunState: " + currentState + ">";
  }
}
