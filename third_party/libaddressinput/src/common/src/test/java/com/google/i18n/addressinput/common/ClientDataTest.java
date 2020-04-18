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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.google.common.collect.ImmutableMap;
import com.google.i18n.addressinput.testing.InMemoryAsyncRequest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.InOrder;
import org.mockito.Mockito;

import java.util.Map;

/**
 * Tests for the ClientData class, to ensure it uses the cache data to correctly fetch data from the
 * server and recovers if no data is present.
 */
@RunWith(JUnit4.class)
public class ClientDataTest {
  private static CacheData createTestCacheData(Map<String, String> keyToJsonMap) {
    InMemoryAsyncRequest inMemoryAsyncRequest =
        new InMemoryAsyncRequest("http://someurl", keyToJsonMap);
    CacheData cacheData = new CacheData(inMemoryAsyncRequest);
    cacheData.setUrl("http://someurl");
    return cacheData;
  }

  @Test public void testGetAlreadyCached() {
    // Start with an empty cache backed by an empty request api.
    CacheData cacheData = createTestCacheData(ImmutableMap.<String, String>of());
    JsoMap testData = new JsoMap();
    cacheData.addToJsoMap("data", testData);
    ClientData client = new ClientData(cacheData);
    assertNotNull(client.get("data"));
  }

  @Test public void testGetRequiresLoading() {
    CacheData cacheData = createTestCacheData(
        ImmutableMap.<String, String>of("data", "{\"id\":\"data\",\"countries\":\"CA~US\"}"));
    ClientData client = new ClientData(cacheData);
    assertNotNull(client.get("data"));
  }

  @Test public void testCacheUpdatesBetweenBeginAndEndCallback() {
    LookupKey key = new LookupKey.Builder("data/CA").build();

    final CacheData cacheData = createTestCacheData(
        ImmutableMap.<String, String>of("data/CA", "{"
            + "\"id\":\"data/CA\","
            + "\"key\":\"CA\","
            + "\"name\":\"CANADA\","
            + "}"));
    ClientData client = new ClientData(cacheData);
    DataLoadListener listener = new DataLoadListener() {
      // Other tests show that these methods are only called once, so a boolean is fine here.
      boolean beginWasCalled = false;
      boolean endWasCalled = false;
      @Override
      public void dataLoadingBegin() {
        assertFalse(cacheData.containsKey("data/CA"));
        beginWasCalled = true;
      }
      @Override
      public void dataLoadingEnd() {
        assertTrue(cacheData.containsKey("data/CA"));
        endWasCalled = true;
      }
      // Cheeky use of toString() to avoid needing a whole new type here to report our state.
      @Override
      public String toString() {
        return beginWasCalled && endWasCalled ? "PASS" : "FAIL";
      }
    };
    assertEquals("FAIL", listener.toString());
    client.requestData(key, listener);
    assertEquals("PASS", listener.toString());
  }

  @Test public void testGetBadDataKey() {
    // Start with an empty cache backed by an empty request api.
    CacheData cacheData = createTestCacheData(ImmutableMap.<String, String>of());
    ClientData client = new ClientData(cacheData);
    assertNull(client.get("xxxx"));
  }

  @Test public void testPrefetchCountry() {
    CacheData cacheData = createTestCacheData(ImmutableMap.<String, String>builder()
        // Very fake data for Taiwan showing a country with a mix of depths to the
        // subregion hierarchy. When preloading data we don't go to the leaf nodes.
        .put("data/TW", "{"
            + "\"id\":\"data/TW\","
            + "\"key\":\"TW\","
            + "\"name\":\"Taiwan\","
            + "\"sub_keys\":\"AA~BB\","
            + "\"sub_names\":\"First~Second\","
            // Indicates which of our children have their own children!
            + "\"sub_mores\":\"true~false\","
            + "}")
        .put("data/TW/AA", "{"
            // AA has children according to sub_mores above.
            + "\"id\":\"data/TW/AA\","
            + "\"key\":\"AA\","
            + "\"name\":\"First\","
            + "\"sub_keys\":\"XX~YY\","
            + "\"sub_names\":\"Foo~Bar\","
            + "}")
        .put("data/TW/AA/XX", "{"
            // Leaf nodes are not loaded.
            + "\"id\":\"data/TW/AA/XX\","
            + "\"key\":\"XX\","
            + "\"name\":\"Foo\","
            + "}")
        .put("data/TW/AA/YY", "{"
            // Leaf nodes are not loaded.
            + "\"id\":\"data/TW/AA/YY\","
            + "\"key\":\"YY\","
            + "\"name\":\"Bar\","
            + "}")
        .put("data/TW/BB", "{"
            // BB does not have children according to sub_mores above, so is not loaded.
            + "\"id\":\"data/TW/BB\","
            + "\"key\":\"BB\","
            + "\"name\":\"Second\","
            + "}")
        .build());

    ClientData client = new ClientData(cacheData);
    DataLoadListener mockListener = Mockito.mock(DataLoadListener.class);
    client.prefetchCountry("TW", mockListener);

    // We preloaded the regions which had sub-keys but none of the leaf nodes.
    assertTrue(cacheData.containsKey("data/TW"));
    assertTrue(cacheData.containsKey("data/TW/AA"));
    assertFalse(cacheData.containsKey("data/TW/AA/XX"));
    assertFalse(cacheData.containsKey("data/TW/BB"));

    InOrder order = Mockito.inOrder(mockListener);
    order.verify(mockListener).dataLoadingBegin();
    order.verify(mockListener).dataLoadingEnd();
  }

  @Test public void testFallbackToBuiltInData() {
    CacheData cacheData = createTestCacheData(ImmutableMap.<String, String>of());
    ClientData client = new ClientData(cacheData);

    AddressVerificationNodeData data = client.get("data/US");

    // No data was available on the server or in the cache - it should check
    // that there is nothing in region data constants, and should return the
    // data from there.
    assertNotNull(data);
    assertEquals("%N%n%O%n%A%n%C, %S %Z", data.get(AddressDataKey.FMT));
  }
}
