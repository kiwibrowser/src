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

package com.google.android.libraries.feed.common.logging;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertEquals;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link Logger}. */
@RunWith(RobolectricTestRunner.class)
public class LoggerTest {
  private static final Object[] EMPTY_ARRAY = {};

  @Test
  public void testBuildMessage_emptyString() {
    assertThat(Logger.buildMessage("", EMPTY_ARRAY)).isEmpty();
  }

  @Test
  public void testBuildMessage_nullString() {
    assertEquals("null", Logger.buildMessage(null, EMPTY_ARRAY));
  }

  @Test
  public void testBuildMessage_nullStringWithArgs() {
    // Test that any args are ignored
    assertEquals("null", Logger.buildMessage(null, new Object[] {"A", "B", 2}));
  }

  @Test
  public void testBuildMessage_noArgs() {
    assertEquals("hello", Logger.buildMessage("hello", EMPTY_ARRAY));
  }

  @Test
  public void testBuildMessage_formatString() {
    String format = "%s scolded %s %d times? %b/%b";
    assertEquals(
        "A scolded B 2 times? true/false",
        Logger.buildMessage(format, new Object[] {"A", "B", 2, true, false}));
  }

  @Test
  public void testBuildMessage_formatStringForceArgsToString() {
    String format = "%s scolded %s %s times? %s/%s";
    assertEquals(
        "A scolded B 2 times? true/false",
        Logger.buildMessage(format, new Object[] {"A", "B", 2, true, false}));
  }

  @Test
  public void testBuildMessage_formatStringNullArgs() {
    String format = "%s scolded %s %d times? %b/%b";
    assertEquals(
        "null scolded null null times? false/false",
        Logger.buildMessage(format, new Object[] {null, null, null, null, null}));
  }

  @Test
  public void testBuildMessage_noFormat() {
    String format = "Hello";
    // Test that args are safely ignored
    assertEquals("Hello", Logger.buildMessage(format, new Object[] {"A", "B", 2}));
  }

  @Test
  public void testBuildMessage_illegalFormat() {
    String format = "Hello %d";
    // Test that args are concatenated when using a string for %d.
    assertEquals("Hello %d[A]", Logger.buildMessage(format, new Object[] {"A"}));
  }

  @Test
  public void testBuildMessage_arrayArgs() {
    String format = "%s %s";
    int[] a = {1, 2, 3};
    Object[] b = {"a", new String[] {"b", "c"}};
    assertEquals("[1, 2, 3] [a, [b, c]]", Logger.buildMessage(format, new Object[] {a, b}));
  }

  @Test
  public void testBuildMessage_arrayArgsWithNull() {
    String format = "%s";
    Object a = new String[] {"a", null};
    assertEquals("[a, null]", Logger.buildMessage(format, new Object[] {a}));
  }

  @Test
  public void testBuildMessage_arrayArgsWithCycle() {
    String format = "%s %s";
    Object[] a = new Object[1];
    Object[] b = {a};
    a[0] = b;
    assertEquals("[[[...]]] [[[...]]]", Logger.buildMessage(format, new Object[] {a, b}));
  }
}
