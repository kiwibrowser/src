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
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class ResultTest {

  @Test
  public void testRequiredParameters() {
    Result result = new Result.Builder().build();
    assertNull(result.getMessageId());
    assertNull(result.getErrorCodeName());
    assertNull(result.getCanonicalRegistrationId());
  }

  @Test
  public void testOptionalParameters() {
    Result result = new Result.Builder()
      .messageId("42")
      .errorCode("D'OH!")
      .canonicalRegistrationId("108")
      .build();
    assertEquals("42", result.getMessageId());
    assertEquals("D'OH!", result.getErrorCodeName());
    assertEquals("108", result.getCanonicalRegistrationId());
    String toString = result.toString();
    assertTrue(toString.contains("messageId=42"));
    assertTrue(toString.contains("errorCode=D'OH!"));
    assertTrue(toString.contains("canonicalRegistrationId=108"));
  }
}
