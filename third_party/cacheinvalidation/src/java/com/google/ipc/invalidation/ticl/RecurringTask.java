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

import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.SystemResources.Scheduler;
import com.google.ipc.invalidation.ticl.proto.Client.ExponentialBackoffState;
import com.google.ipc.invalidation.ticl.proto.JavaClient.RecurringTaskState;
import com.google.ipc.invalidation.util.ExponentialBackoffDelayGenerator;
import com.google.ipc.invalidation.util.InternalBase;
import com.google.ipc.invalidation.util.Marshallable;
import com.google.ipc.invalidation.util.NamedRunnable;
import com.google.ipc.invalidation.util.Preconditions;
import com.google.ipc.invalidation.util.Smearer;
import com.google.ipc.invalidation.util.TextBuilder;


/**
 * An abstraction for scheduling recurring tasks. Combines idempotent scheduling and smearing with
 * conditional retries and exponential backoff. Does not implement throttling. Designed to support a
 * variety of use cases, including:
 *
 * <ul>
 * <li>Idempotent scheduling, e.g., ensuring that a batching task is scheduled exactly once.
 * <li>Recurring tasks, e.g., periodic heartbeats.
 * <li>Retriable actions aimed at state change, e.g., sending initialization messages.
 * </ul>
 * Each instance of this class manages the state for a single task. Examples:
 *
 * <pre>
 * batchingTask = new RecurringTask("Batching", scheduler, logger, smearer, null,
 *   batchingDelayMs, NO_DELAY) {
 *  @Override
 *  public boolean runTask() {
 *    throttle.fire();
 *    return false;  // don't reschedule.
 *  }
 * };
 * heartbeatTask = new RecurringTask("Heartbeat", scheduler, logger, smearer, null,
 *   heartbeatDelayMs, NO_DELAY) {
 *  @Override
 *  public boolean runTask() {
 *    sendInfoMessageToServer(false, !registrationManager.isStateInSyncWithServer());
 *    return true;  // reschedule
 *  }
 * };
 * initializeTask = new RecurringTask("Token", scheduler, logger, smearer, expDelayGen, NO_DELAY,
 *    networkTimeoutMs) {
 *  @Override
 *  public boolean runTask() {
 *   // If token is still not assigned (as expected), sends a request. Otherwise, ignore.
 *   if (clientToken == null) {
 *     // Allocate a nonce and send a message requesting a new token.
 *     setNonce(ByteString.copyFromUtf8(Long.toString(internalScheduler.getCurrentTimeMs())));
 *     protocolHandler.sendInitializeMessage(applicationClientId, nonce, debugString);
 *     return true;  // reschedule to check state, retry if necessary after timeout
 *    } else {
 *     return false;  // don't reschedule
 *    }
 *  }
 * };
 *</pre>
 *
 */
public abstract class RecurringTask extends InternalBase
    implements Marshallable<RecurringTaskState> {

  /** Name of the task (for debugging purposes mostly). */
  private final String name;

  /** A logger */
  private final Logger logger;

  /** Scheduler for the scheduling the task as needed. */
  private final Scheduler scheduler;

  /**
   * The time after which the task is scheduled first. If no delayGenerator is specified, this is
   * also the delay used for retries.
   */
  private final int initialDelayMs;

  /** For a task that is retried, add this time to the delay. */
  private final int timeoutDelayMs;

  /** A smearer for spreading the delays. */
  private final Smearer smearer;

  /** A delay generator for exponential backoff. */
  private final TiclExponentialBackoffDelayGenerator delayGenerator;

  /** The runnable that is scheduled for the task. */
  private final NamedRunnable runnable;

  /** If the task has been currently scheduled. */
  private boolean isScheduled;

  /**
   * Creates a recurring task with the given parameters. The specs of the parameters are given in
   * the instance variables.
   * <p>
   * The created task is first scheduled with a smeared delay of {@code initialDelayMs}. If the
   * {@code this.run()} returns true on its execution, the task is rescheduled after a
   * {@code timeoutDelayMs} + smeared delay of {@code initialDelayMs} or {@code timeoutDelayMs} +
   * {@code delayGenerator.getNextDelay()} depending on whether the {@code delayGenerator} is null
   * or not.
   */
  
  public RecurringTask(String name, Scheduler scheduler, Logger logger, Smearer smearer,
      TiclExponentialBackoffDelayGenerator delayGenerator,
      final int initialDelayMs, final int timeoutDelayMs) {
    this.delayGenerator = delayGenerator;
    this.name = Preconditions.checkNotNull(name);
    this.logger = Preconditions.checkNotNull(logger);
    this.scheduler = Preconditions.checkNotNull(scheduler);
    this.smearer = Preconditions.checkNotNull(smearer);
    this.initialDelayMs = initialDelayMs;
    this.isScheduled = false;
    this.timeoutDelayMs = timeoutDelayMs;

    // Create a runnable that runs the task. If the task asks for a retry, reschedule it after
    // at a timeout delay. Otherwise, resets the delayGenerator.
    this.runnable = createRunnable();
  }

  /**
   * Creates a recurring task from {@code marshalledState}. Other parameters are as in the
   * constructor above.
   */
  RecurringTask(String name, Scheduler scheduler, Logger logger, Smearer smearer,
      TiclExponentialBackoffDelayGenerator delayGenerator,
      RecurringTaskState marshalledState) {
    this(name, scheduler, logger, smearer, delayGenerator, marshalledState.getInitialDelayMs(),
        marshalledState.getTimeoutDelayMs());
    this.isScheduled = marshalledState.getScheduled();
  }

  private NamedRunnable createRunnable() {
    return new NamedRunnable(name) {
      @Override
      public void run() {
        Preconditions.checkState(scheduler.isRunningOnThread(), "Not on scheduler thread");
        isScheduled = false;
        if (runTask()) {
          // The task asked to be rescheduled, so reschedule it after a timeout has occurred.
          Preconditions.checkState((delayGenerator != null) || (initialDelayMs != 0),
              "Spinning: No exp back off and initialdelay is zero");
          ensureScheduled(true, "Retry");
        } else if (delayGenerator != null) {
          // The task asked not to be rescheduled.  Treat it as having "succeeded" and reset the
          // delay generator.
          delayGenerator.reset();
        }
      }
    };
  }

  /**
   * Run the task and return true if the task should be rescheduled after a timeout. If false is
   * returned, the task is not scheduled again until {@code ensureScheduled} is called again.
   */
  public abstract boolean runTask();

  /** Returns the smearer used for randomizing delays. */
  Smearer getSmearer() {
    return smearer;
  }

  /** Returns the delay generator, if any. */
  ExponentialBackoffDelayGenerator getDelayGenerator() {
    return delayGenerator;
  }

  /**
   * Ensures that the task is scheduled (with {@code debugReason} as the reason to be printed
   * for debugging purposes). If the task has been scheduled, it is not scheduled again.
   * <p>
   * REQUIRES: Must be called from the scheduler thread.
   */
  
  public void ensureScheduled(String debugReason) {
    ensureScheduled(false, debugReason);
  }

  /**
   * Ensures that the task is scheduled if it is already not scheduled. If already scheduled, this
   * method is a no-op.
   *
   * @param isRetry If this is {@code false}, smears the {@code initialDelayMs} and uses that delay
   *        for scheduling. If {@code isRetry} is true, it determines the new delay to be
   *        {@code timeoutDelayMs} + {@ocde delayGenerator.getNextDelay()} if
   *        {@code delayGenerator} is non-null. If {@code delayGenerator} is null, schedules the
   *        task after a delay of {@code timeoutDelayMs} + smeared value of {@code initialDelayMs}
   * <p>
   * REQUIRES: Must be called from the scheduler thread.
   */
  private void ensureScheduled(boolean isRetry, String debugReason) {
    Preconditions.checkState(scheduler.isRunningOnThread());
    if (isScheduled) {
      logger.fine("[%s] Not scheduling task that is already scheduled", debugReason);
      return;
    }
    final int delayMs;

    if (isRetry) {
      // For a retried task, determine the delay to be timeout + extra delay (depending on whether
      // a delay generator was provided or not).
      if (delayGenerator != null) {
        delayMs = timeoutDelayMs + delayGenerator.getNextDelay();
      } else {
        delayMs = timeoutDelayMs + smearer.getSmearedDelay(initialDelayMs);
      }
    } else {
      delayMs = smearer.getSmearedDelay(initialDelayMs);
    }

    logger.fine("[%s] Scheduling %s with a delay %s, Now = %s", debugReason, name, delayMs,
        scheduler.getCurrentTimeMs());
    scheduler.schedule(delayMs, runnable);
    isScheduled = true;
  }

  /** For use only in the Android scheduler. */
  public NamedRunnable getRunnable() {
    return runnable;
  }

  @Override
  public RecurringTaskState marshal() {
    ExponentialBackoffState backoffState =
        (delayGenerator == null) ? null : delayGenerator.marshal();
    return RecurringTaskState.create(initialDelayMs, timeoutDelayMs, isScheduled, backoffState);
  }

  @Override
  public void toCompactString(TextBuilder builder) {
    builder.append("<RecurringTask: name=").append(name)
        .append(", initialDelayMs=").append(initialDelayMs)
        .append(", timeoutDelayMs=").append(timeoutDelayMs)
        .append(", isScheduled=").append(isScheduled)
        .append(">");
  }
}
