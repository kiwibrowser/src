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
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

import java.util.Arrays;
import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public class MulticastResultTest {

  @Test
  public void testRequiredParameters_noResults() {
    MulticastResult multicastResult = new MulticastResult.Builder(4, 8, 15, 16)
        .build();
    assertEquals(4, multicastResult.getSuccess());
    assertEquals(8, multicastResult.getFailure());
    assertEquals(12, multicastResult.getTotal());
    assertEquals(16, multicastResult.getMulticastId());
    assertTrue(multicastResult.getResults().isEmpty());
    assertTrue(multicastResult.getRetryMulticastIds().isEmpty());
  }

  @Test
  public void testRequiredParameters_withResults() {
    MulticastResult multicastResult = new MulticastResult.Builder(4, 8, 15, 16)
        .addResult(new Result.Builder().messageId("23").build())
        .addResult(new Result.Builder().messageId("42").build())
        .build();
    assertEquals(4, multicastResult.getSuccess());
    assertEquals(8, multicastResult.getFailure());
    assertEquals(12, multicastResult.getTotal());
    assertEquals(16, multicastResult.getMulticastId());
    List<Result> results = multicastResult.getResults();
    assertEquals(2, results.size());
    assertEquals("23", results.get(0).getMessageId());
    assertEquals("42", results.get(1).getMessageId());
    String toString = multicastResult.toString();
    assertTrue(toString.contains("multicast_id=16"));
    assertTrue(toString.contains("total=12"));
    assertTrue(toString.contains("success=4"));
    assertTrue(toString.contains("failure=8"));
    assertTrue(toString.contains("canonical_ids=15"));
    assertTrue(toString.contains("results"));
  }

  @Test
  public void testOptionalParameters() {
    MulticastResult multicastResult = new MulticastResult.Builder(4, 8, 15, 16)
        .retryMulticastIds(Arrays.asList(23L, 42L))
        .build();
    assertEquals(4, multicastResult.getSuccess());
    assertEquals(8, multicastResult.getFailure());
    assertEquals(12, multicastResult.getTotal());
    assertEquals(16, multicastResult.getMulticastId());
    assertTrue(multicastResult.getResults().isEmpty());
    List<Long> retryMulticastIds = multicastResult.getRetryMulticastIds();
    assertEquals(2, retryMulticastIds.size());
    assertEquals(23L, retryMulticastIds.get(0).longValue());
    assertEquals(42L, retryMulticastIds.get(1).longValue());
  }

  @Test(expected = UnsupportedOperationException.class)
  public void testResultsIsImmutable() {
    MulticastResult result = new MulticastResult.Builder(1, 2, 3, 4).build();
    result.getResults().clear();
  }

  @Test(expected = UnsupportedOperationException.class)
  public void testRetryMulticastIdsIsImmutable() {
    MulticastResult result = new MulticastResult.Builder(1, 2, 3, 4).build();
    result.getRetryMulticastIds().clear();
  }
}
