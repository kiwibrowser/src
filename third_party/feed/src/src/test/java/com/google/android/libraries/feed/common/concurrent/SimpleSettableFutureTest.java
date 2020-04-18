// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.common.concurrent;

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;

import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link SimpleSettableFuture} */
@RunWith(RobolectricTestRunner.class)
public class SimpleSettableFutureTest {

  @Test
  public void testCancel() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    boolean cancelled = future.cancel(false);
    assertThat(cancelled).isTrue();
    assertThatRunnable(future::get)
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(CancellationException.class);
    assertThat(future.isCancelled()).isTrue();
    assertThat(future.isDone()).isTrue();
  }

  @Test
  public void testCancel_multipleCalls() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    boolean cancelled = future.cancel(false);
    cancelled = future.cancel(false);
    assertThat(cancelled).isTrue();
    assertThatRunnable(future::get)
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(CancellationException.class);
    assertThat(future.isCancelled()).isTrue();
    assertThat(future.isDone()).isTrue();
  }

  @Test
  public void testCancel_alreadyFinished() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.put(Boolean.TRUE);
    boolean cancelled = future.cancel(false);
    assertThat(cancelled).isFalse();
  }

  @Test
  public void testPut() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.put(Boolean.TRUE);
    Boolean futureResult = future.get();
    assertThat(futureResult).isEqualTo(Boolean.TRUE);
    assertThat(future.isDone()).isTrue();
  }

  @Test
  public void testPut_onlyOnce() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.put(Boolean.TRUE);
    future.put(Boolean.FALSE);
    Boolean futureResult = future.get();
    assertThat(futureResult).isEqualTo(Boolean.TRUE);
    assertThat(future.isDone()).isTrue();
  }

  @Test
  public void testPut_onlyOnce_andException() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.put(Boolean.TRUE);
    future.putException(new RuntimeException());
    Boolean futureResult = future.get();
    assertThat(futureResult).isEqualTo(Boolean.TRUE);
    assertThat(future.isDone()).isTrue();
  }

  @Test
  public void testPut_alreadyCancelled() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    boolean cancelled = future.cancel(false);
    future.put(Boolean.TRUE);
    assertThatRunnable(future::get)
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(CancellationException.class);
    assertThat(future.isCancelled()).isTrue();
    assertThat(future.isDone()).isTrue();
  }

  @Test
  public void testPutException() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.putException(new RuntimeException());
    assertThatRunnable(future::get)
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(RuntimeException.class);
  }

  @Test
  public void testPutException_onlyOnce() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.putException(new RuntimeException());
    future.putException(new IllegalAccessException());
    assertThatRunnable(future::get)
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(RuntimeException.class);
  }

  @Test
  public void testPutException_onlyOnce_andValue() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.putException(new RuntimeException());
    future.put(Boolean.TRUE);
    assertThatRunnable(future::get)
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(RuntimeException.class);
  }

  @Test
  public void testPutException_alreadyCancelled() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    boolean cancelled = future.cancel(false);
    future.putException(new RuntimeException());
    assertThatRunnable(future::get)
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(CancellationException.class);
    assertThat(future.isCancelled()).isTrue();
    assertThat(future.isDone()).isTrue();
  }

  @Test
  public void testGetTimeout() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    assertThatRunnable(() -> future.get(1, TimeUnit.MILLISECONDS))
        .throwsAnExceptionOfType(TimeoutException.class);
  }

  @Test
  public void testGetTimeout_success() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.put(Boolean.TRUE);
    Boolean futureResult = future.get(1, TimeUnit.MILLISECONDS);
    assertThat(futureResult).isEqualTo(Boolean.TRUE);
    assertThat(future.isDone()).isTrue();
  }

  @Test
  public void testGetTimeout_cancelled() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.cancel(false);
    assertThatRunnable(() -> future.get(1, TimeUnit.MILLISECONDS))
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(CancellationException.class);
  }

  @Test
  public void testGetTimeout_putException() throws Exception {
    SimpleSettableFuture<Boolean> future = new SimpleSettableFuture<>();
    future.putException(new RuntimeException());
    assertThatRunnable(() -> future.get(1, TimeUnit.MILLISECONDS))
        .throwsAnExceptionOfType(ExecutionException.class)
        .causedByAnExceptionOfType(RuntimeException.class);
  }
}
