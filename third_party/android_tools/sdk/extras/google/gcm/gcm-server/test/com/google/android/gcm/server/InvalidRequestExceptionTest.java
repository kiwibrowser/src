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
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class InvalidRequestExceptionTest {

  @Test
  public void testGetters_noDescription() {
    InvalidRequestException exception = new InvalidRequestException(401);
    assertEquals(401, exception.getHttpStatusCode());
    assertNull(exception.getDescription());
    assertTrue(exception.getMessage().contains("401"));
  }

  @Test
  public void testGetters_description() {
    InvalidRequestException exception =
        new InvalidRequestException(401, "D'OH!");
    assertEquals(401, exception.getHttpStatusCode());
    assertEquals("D'OH!", exception.getDescription());
    assertTrue(exception.getMessage().contains("401"));
    assertTrue(exception.getMessage().contains("D'OH!"));
  }
}
