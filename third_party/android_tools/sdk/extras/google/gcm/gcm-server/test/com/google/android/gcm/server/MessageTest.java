/*
 * Copyright 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.gcm.server;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

import java.util.Map;

@RunWith(MockitoJUnitRunner.class)
public class MessageTest {

  @Test
  public void testRequiredParameters() {
    Message message = new Message.Builder().build();
    assertNull(message.getCollapseKey());
    assertNull(message.isDelayWhileIdle());
    assertTrue(message.getData().isEmpty());
    assertNull(message.getTimeToLive());
    String toString = message.toString();
    assertFalse(toString.contains("collapseKey"));
    assertFalse(toString.contains("timeToLive"));
    assertFalse(toString.contains("delayWhileIdle"));
    assertFalse(toString.contains("data"));
  }

  @Test
  public void testOptionalParameters() {
    Message message = new Message.Builder()
        .collapseKey("108")
        .delayWhileIdle(true)
        .timeToLive(42)
        .addData("k1", "old value")
        .addData("k1", "v1")
        .addData("k2", "v2")
        .build();
    assertEquals("108", message.getCollapseKey());
    assertTrue(message.isDelayWhileIdle());
    assertEquals(42, message.getTimeToLive().intValue());
    Map<String, String> data = message.getData();
    assertEquals(2, data.size());
    assertEquals("v1", data.get("k1"));
    assertEquals("v2", data.get("k2"));
    String toString = message.toString();
    assertTrue(toString.contains("collapseKey=108"));
    assertTrue(toString.contains("timeToLive=42"));
    assertTrue(toString.contains("delayWhileIdle=true"));
    assertTrue(toString.contains("k1=v1"));
    assertTrue(toString.contains("k2=v2"));
  }

  @Test(expected = UnsupportedOperationException.class)
  public void testPayloadDataIsImmutable() {
    Message message = new Message.Builder().build();
    message.getData().clear();
  }
}
