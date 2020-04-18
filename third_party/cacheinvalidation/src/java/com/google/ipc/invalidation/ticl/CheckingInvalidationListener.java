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

import static com.google.ipc.invalidation.external.client.SystemResources.Scheduler.NO_DELAY;

import com.google.ipc.invalidation.external.client.InvalidationClient;
import com.google.ipc.invalidation.external.client.InvalidationListener;
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.SystemResources.Scheduler;
import com.google.ipc.invalidation.external.client.types.AckHandle;
import com.google.ipc.invalidation.external.client.types.ErrorInfo;
import com.google.ipc.invalidation.external.client.types.Invalidation;
import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.ipc.invalidation.ticl.Statistics.ListenerEventType;
import com.google.ipc.invalidation.util.NamedRunnable;
import com.google.ipc.invalidation.util.Preconditions;


/**
 * {@link InvalidationListener} wrapper that ensures that a delegate listener is called on the
 * proper thread and calls the listener method on the listener thread.
 *
 */
class CheckingInvalidationListener implements InvalidationListener {

  /** The actual listener to which this listener delegates. */
  private final InvalidationListener delegate;

  /** The scheduler for scheduling internal events in the library. */
  private final Scheduler internalScheduler;

  /** The scheduler for scheduling events for the delegate. */
  private final Scheduler listenerScheduler;

  /** Statistics objects to track number of sent messages, etc. */
  private Statistics statistics;

  private final Logger logger;

  CheckingInvalidationListener(InvalidationListener delegate, Scheduler internalScheduler,
      Scheduler listenerScheduler, Logger logger) {
    this.delegate = Preconditions.checkNotNull(delegate, "Delegate cannot be null");
    this.internalScheduler = Preconditions.checkNotNull(internalScheduler,
        "Internal scheduler cannot be null");
    this.listenerScheduler = Preconditions.checkNotNull(listenerScheduler,
        "Listener scheduler cannot be null");
    this.logger = Preconditions.checkNotNull(logger, "Logger cannot be null");
  }

  void setStatistics(Statistics statistics) {
    this.statistics = Preconditions.checkNotNull(statistics, "Statistics cannot be null");
  }

  @Override
  public void invalidate(final InvalidationClient client, final Invalidation invalidation,
      final AckHandle ackHandle) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    Preconditions.checkNotNull(ackHandle);
    listenerScheduler.schedule(NO_DELAY, new NamedRunnable("CheckingInvalListener.invalidate") {
      @Override
      public void run() {
        statistics.recordListenerEvent(ListenerEventType.INVALIDATE);
        delegate.invalidate(client, invalidation, ackHandle);
      }
    });
  }

  @Override
  public void invalidateUnknownVersion(final InvalidationClient client, final ObjectId objectId,
      final AckHandle ackHandle) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    Preconditions.checkNotNull(ackHandle);
    listenerScheduler.schedule(NO_DELAY,
        new NamedRunnable("CheckingInvalListener.invalidateUnknownVersion") {
      @Override
      public void run() {
        statistics.recordListenerEvent(ListenerEventType.INVALIDATE_UNKNOWN);
        delegate.invalidateUnknownVersion(client, objectId, ackHandle);
      }
    });
  }

  @Override
  public void invalidateAll(final InvalidationClient client, final AckHandle ackHandle) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    Preconditions.checkNotNull(ackHandle);
    listenerScheduler.schedule(NO_DELAY, new NamedRunnable("CheckingInvalListener.invalidateAll") {
      @Override
      public void run() {
        statistics.recordListenerEvent(ListenerEventType.INVALIDATE_ALL);
        delegate.invalidateAll(client, ackHandle);
      }
    });
  }

  @Override
  public void informRegistrationFailure(final InvalidationClient client, final ObjectId objectId,
      final boolean isTransient, final String errorMessage) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    listenerScheduler.schedule(NO_DELAY, new NamedRunnable("CheckingInvalListener.regFailure") {
      @Override
      public void run() {
        statistics.recordListenerEvent(ListenerEventType.INFORM_REGISTRATION_FAILURE);
        delegate.informRegistrationFailure(client, objectId, isTransient, errorMessage);
      }
    });
  }

  @Override
  public void informRegistrationStatus(final InvalidationClient client, final ObjectId objectId,
      final RegistrationState regState) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    listenerScheduler.schedule(NO_DELAY, new NamedRunnable("CheckingInvalListener.regStatus") {
      @Override
      public void run() {
        statistics.recordListenerEvent(ListenerEventType.INFORM_REGISTRATION_STATUS);
        delegate.informRegistrationStatus(client, objectId, regState);
      }
    });
  }

  @Override
  public void reissueRegistrations(final InvalidationClient client, final byte[] prefix,
      final int prefixLen) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    listenerScheduler.schedule(NO_DELAY, new NamedRunnable("CheckingInvalListener.reissueRegs") {
      @Override
      public void run() {
        statistics.recordListenerEvent(ListenerEventType.REISSUE_REGISTRATIONS);
        delegate.reissueRegistrations(client, prefix, prefixLen);
      }
    });
  }

  @Override
  public void informError(final InvalidationClient client, final ErrorInfo errorInfo) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    listenerScheduler.schedule(NO_DELAY, new NamedRunnable("CheckingInvalListener.informError") {
      @Override
      public void run() {
        statistics.recordListenerEvent(ListenerEventType.INFORM_ERROR);
        delegate.informError(client, errorInfo);
      }
    });
  }

  /** Returns the delegate {@link InvalidationListener}. */
  InvalidationListener getDelegate() {
    return delegate;
  }

  @Override
  public void ready(final InvalidationClient client) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    listenerScheduler.schedule(NO_DELAY, new NamedRunnable("CheckingInvalListener.ready") {
      @Override
      public void run() {
        logger.info("Informing app that ticl is ready");
        delegate.ready(client);
      }
    });
  }
}
