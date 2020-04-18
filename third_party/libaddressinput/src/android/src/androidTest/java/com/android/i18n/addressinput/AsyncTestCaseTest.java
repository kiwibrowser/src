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

package com.android.i18n.addressinput;

import com.android.i18n.addressinput.testing.AsyncTestCase;

import junit.framework.AssertionFailedError;

import java.util.concurrent.TimeoutException;

public class AsyncTestCaseTest extends AsyncTestCase {

  public void testSuccess() {
    delayTestFinish(1000);
    AsyncCallback.execute(500, new Runnable() {
      @Override
      public void run() {
        finishTest();
      }
    });
  }

  public void testFailure() {
    expectTimeout = true;
    delayTestFinish(1000);
    AsyncCallback.execute(1500, new Runnable() {
      @Override
      public void run() {
        finishTest();
      }
    });
  }

  @Override
  protected void runTest() throws Throwable {
    expectTimeout = false;
    try {
      super.runTest();
    } catch (TimeoutException e) {
      if (expectTimeout) {
        return;
      } else {
        throw e;
      }
    }
    if (expectTimeout) {
      throw new AssertionFailedError("Test case did not time out.");
    }
  }

  private boolean expectTimeout;

  /**
   * Helper class to perform an asynchronous callback after a specified delay.
   */
  private static class AsyncCallback extends Thread {
    private long waitMillis;
    private Runnable callback;

    private AsyncCallback(long waitMillis, Runnable callback) {
      this.waitMillis = waitMillis;
      this.callback = callback;
    }

    public static void execute(long waitMillis, Runnable callback) {
      (new AsyncCallback(waitMillis, callback)).start();
    }

    @Override
    public void run() {
      try {
        synchronized (this) {
          wait(this.waitMillis);
        }
      } catch (InterruptedException e) {
        throw new RuntimeException(e);
      }
      this.callback.run();
    }
  }
}
