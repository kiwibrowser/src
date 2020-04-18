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

package com.google.android.libraries.feed.common.testing;

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.android.libraries.feed.common.testing.RunnableSubject.runnables;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import com.google.android.libraries.feed.common.testing.RunnableSubject.ThrowingRunnable;
import com.google.common.truth.ExpectFailure;
import com.google.common.truth.Truth;
import java.io.IOException;
import java.util.concurrent.ExecutionException;
import org.junit.ComparisonFailure;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link RunnableSubject}. */
@RunWith(JUnit4.class)
public class RunnableSubjectTest {
  @Rule public final ExpectFailure expectFailure = new ExpectFailure();

  @Test
  public void runToCompletion() throws Throwable {
    ThrowingRunnable run = mock(ThrowingRunnable.class);

    expectFailure
        .whenTesting()
        .about(runnables())
        .that(run)
        .throwsAnExceptionOfType(RuntimeException.class);

    verify(run).run();
    assertThat(expectFailure.getFailure())
        .hasMessageThat()
        .contains(" threw a <java.lang.RuntimeException>. It ran to <completion>");
    verifyNoMoreInteractions(run);
  }

  @Test
  public void nullSubjectFailsEvenWhenExpectingNullRefException() {
    expectFailure
        .whenTesting()
        .about(runnables())
        .that(null)
        .throwsAnExceptionOfType(NullPointerException.class);

    assertThat(expectFailure.getFailure())
        .hasMessageThat()
        .contains(" threw a <java.lang.NullPointerException>. It couldn't run because it's <null>");
  }

  @Test
  public void wrongException() {
    expectFailure
        .whenTesting()
        .about(runnables())
        .that(
            () -> {
              throw new IllegalArgumentException("a");
            })
        .throwsAnExceptionOfType(NullPointerException.class);

    assertThat(expectFailure.getFailure())
        .hasMessageThat()
        .contains(
            " threw a <java.lang.NullPointerException>. "
                + "It threw <java.lang.IllegalArgumentException");
  }

  @Test
  public void correctException() {
    IllegalArgumentException ex = new IllegalArgumentException("b");

    Truth.assertAbout(runnables())
        .that(
            () -> {
              throw ex;
            })
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .isSameAs(ex);
  }

  @Test
  public void wrongMessage() {
    expectFailure
        .whenTesting()
        .about(runnables())
        .that(
            () -> {
              throw new IOException("Wrong message!");
            })
        .throwsAnExceptionOfType(IOException.class)
        .that()
        .hasMessageThat()
        .isEqualTo("Expected message.");

    ComparisonFailure failure = (ComparisonFailure) expectFailure.getFailure();
    assertThat(failure.getExpected()).isEqualTo("Expected message.");
    assertThat(failure.getActual()).isEqualTo("Wrong message!");
  }

  @Test
  public void correctMessage() {
    String expectedMessage = "Expected message.";

    Truth.assertAbout(runnables())
        .that(
            () -> {
              throw new IOException(expectedMessage);
            })
        .throwsAnExceptionOfType(IOException.class)
        .that()
        .hasMessageThat()
        .isEqualTo(expectedMessage);
  }

  @Test
  public void getCaught() {
    IllegalArgumentException cause = new IllegalArgumentException("boo");
    IOException ex = new IOException(cause);

    IOException caught =
        RunnableSubject.assertThat(
                () -> {
                  throw ex;
                })
            .throwsAnExceptionOfType(IOException.class)
            .getCaught();
    assertThat(caught).isSameAs(ex);
  }

  @Test
  public void wrongCauseType() {
    IllegalArgumentException cause = new IllegalArgumentException("boo");
    IOException ex = new IOException(cause);
    expectFailure
        .whenTesting()
        .about(runnables())
        .that(
            () -> {
              throw ex;
            })
        .throwsAnExceptionOfType(IOException.class)
        .causedByAnExceptionOfType(ExecutionException.class);
    assertThat(expectFailure.getFailure())
        .hasMessageThat()
        .contains(
            " threw an exception caused by <java.util.concurrent.ExecutionException>. "
                + "It was caused by <java.lang.IllegalArgumentException");
  }

  @Test
  public void correctCauseType() {
    IllegalArgumentException cause = new IllegalArgumentException("boo");
    IOException ex = new IOException(cause);

    Truth.assertAbout(runnables())
        .that(
            () -> {
              throw ex;
            })
        .throwsAnExceptionOfType(IOException.class)
        .causedByAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .isEqualTo("boo");
  }

  @Test
  public void causedByCorrect() {
    IOException cause = new IOException();
    IllegalArgumentException ex = new IllegalArgumentException(cause);

    Truth.assertAbout(runnables())
        .that(
            () -> {
              throw ex;
            })
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .causedBy(cause);
  }

  @Test
  public void causedByWrong() {
    IOException cause = new IOException();
    IllegalArgumentException ex = new IllegalArgumentException(cause);

    expectFailure
        .whenTesting()
        .about(runnables())
        .that(
            () -> {
              throw ex;
            })
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .causedBy(new IOException());
  }

  @Test
  public void exampleUsage() {
    assertThatRunnable(
            () -> {
              throw new IOException("boo!");
            })
        .throwsAnExceptionOfType(IOException.class)
        .that()
        .hasMessageThat()
        .isEqualTo("boo!");

    RunnableSubject.assertThat(
            () -> {
              throw new IllegalArgumentException("oh no");
            })
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .isEqualTo("oh no");

    RunnableSubject.assertThat(
            () -> {
              throw new IllegalArgumentException(new RuntimeException("glitch"));
            })
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .causedByAnExceptionOfType(RuntimeException.class)
        .that()
        .hasMessageThat()
        .isEqualTo("glitch");

    // Recursive self-verification.
    RunnableSubject.assertThat(
            () ->
                RunnableSubject.assertThat(() -> {})
                    .throwsAnExceptionOfType(RuntimeException.class))
        .throwsAnExceptionOfType(AssertionError.class);
  }
}
