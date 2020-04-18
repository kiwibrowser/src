/*
 * Copyright (C) 2010 Google Inc.
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

package com.google.i18n.addressinput.common;

/**
 * A helper class to let the calling thread wait until loading has finished.
 */
// TODO: Consider dealing with interruption in a more recoverable way.
public final class NotifyingListener implements DataLoadListener {
  private boolean done = false;

  @Override
  public void dataLoadingBegin() {
  }

  @Override
  public synchronized void dataLoadingEnd() {
    done = true;
    notifyAll();
  }

  /**
   * Waits for a call to {@link #dataLoadingEnd} to have occurred. If this thread is interrupted,
   * the {@code InterruptedException} is propagated immediately and the loading may not yet have
   * finished. This leaves callers in a potentially unrecoverable state.
   */
  public synchronized void waitLoadingEnd() throws InterruptedException {
    while (!done) {
      wait();
    }
  }
}
