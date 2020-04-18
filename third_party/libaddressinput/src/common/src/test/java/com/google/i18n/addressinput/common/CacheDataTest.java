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
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.google.common.base.Preconditions;
import com.google.i18n.addressinput.common.AsyncRequestApi.AsyncCallback;
import com.google.i18n.addressinput.testing.AddressDataMapLoader;

import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.InOrder;
import org.mockito.Mockito;

import java.util.ArrayList;
import java.util.List;

@RunWith(JUnit4.class)
public class CacheDataTest {
  private static final String DELIM = "~";

  private static final String CANADA_KEY = "data/CA";

  private static final String CALIFORNIA_KEY = "data/US/CA";

  private static final String INVALID_KEY = "data/asIOSDxcowW";

  private static final AsyncRequestApi EXPLODING_REQUEST_API = new AsyncRequestApi() {
    @Override
    public void requestObject(String url, AsyncCallback callback, int timeoutMillis) {
      throw new AssertionError("unexpected use of AsyncRequestApi");
    }
  };

  /**
   * This is a simpler mock implementation distinct from the InMemoryAsyncReuqest which
   * lets us test the asynchronous logic around data loading for a single expected value
   * (unlike InMemoryAsyncReuqest, which tests fake data that loads immediately).
   */
  private static class MockAsyncRequest implements AsyncRequestApi {
    private final JsoMap jsoMap;
    private AsyncCallback callback = null;

    MockAsyncRequest(String jsonString) {
      try {
        this.jsoMap = JsoMap.buildJsoMap(jsonString);
      } catch (JSONException e) {
        throw new AssertionError("Invalid test JSON data: " + jsonString, e);
      }
    }

    @Override
    public void requestObject(String url, AsyncCallback callback, int timeoutMillis) {
      Preconditions.checkState(this.callback == null, "requestObject was invoked twice");
      this.callback = Preconditions.checkNotNull(callback);
    }

    void receiveResponse() {
      Preconditions.checkState(callback != null, "requestObject was not invoked");
      callback.onSuccess(jsoMap);
    }

    void timeoutWith() {
      Preconditions.checkState(callback != null, "requestObject was not invoked");
      callback.onFailure();
    }
  }

  private static void assertListenerWasCalled(DataLoadListener mockListener) {
    InOrder order = Mockito.inOrder(mockListener);
    order.verify(mockListener).dataLoadingBegin();
    order.verify(mockListener).dataLoadingEnd();
  }

  @Test public void testJsonConstructor() {
    // Creating cache with content.
    String id = "data/CA";
    JSONObject jsonObject = null;
    try {
      jsonObject = new JSONObject(AddressDataMapLoader.TEST_COUNTRY_DATA.get(id));
    } catch (JSONException jsonException) {
      // If this throws an exception the test fails.
      fail("Can't parse json object");
    }

    CacheData cache = new CacheData(EXPLODING_REQUEST_API);

    cache.addToJsoMap(id, jsonObject);
    String toBackup = cache.getJsonString();

    // Creating cache from saved data.
    cache = new CacheData(toBackup, EXPLODING_REQUEST_API);
    assertTrue(cache.containsKey(id));
  }

  @Test public void testJsonConstructorTruncatedProperString() {
    // Creating cache with content.
    CacheData cache = new CacheData(EXPLODING_REQUEST_API);
    String id = "data/CA";
    try {
      JSONObject jsonObject = new JSONObject(AddressDataMapLoader.TEST_COUNTRY_DATA.get(id));
      String jsonString = jsonObject.toString();
      jsonString = jsonString.substring(0, jsonString.length() / 2);

      cache = new CacheData(jsonString, EXPLODING_REQUEST_API);
      assertTrue(cache.toString(), cache.isEmpty());
    } catch (JSONException jsonException) {
      // If this throws an exception the test fails.
      fail("Can't parse json object");
    }
  }

  @Test public void testSimpleFetching() {
    LookupKey key = new LookupKey.Builder(CANADA_KEY).build();

    MockAsyncRequest fakeRequestApi = new MockAsyncRequest("{"
        + "\"id\":\"data/CA\","
        + "\"key\":\"CA\","
        + "\"name\":\"CANADA\","
        + "\"lang\":\"en\","
        + "\"languages\":\"en~fr\","
        + "\"fmt\":\"%N%n%O%n%A%n%C %S %Z\","
        + "\"require\":\"ACSZ\","
        + "\"upper\":\"ACNOSZ\","
        + "\"zip\":\"[A-Z]\\\\d[A-Z][ ]?\\\\d[A-Z]\\\\d\","
        + "\"zipex\":\"H3Z 2Y7,V8X 3X4\","
        + "\"posturl\":\"http://www.canadapost.ca\","
        + "\"sub_keys\":\"AB~BC~YT\","
        + "\"sub_names\":\"Alberta~British Columbia~Yukon\","
        + "\"sub_zips\":\"T~V~Y\""
        + "}");
    CacheData cache = new CacheData(fakeRequestApi);

    DataLoadListener mockListener = Mockito.mock(DataLoadListener.class);
    cache.fetchDynamicData(key, null, mockListener);
    // Force response to be received here...
    fakeRequestApi.receiveResponse();
    assertListenerWasCalled(mockListener);

    JsoMap map = cache.getObj(CANADA_KEY);
    assertTrue(map.containsKey("id"));
    assertTrue(map.containsKey("lang"));
    assertTrue(map.containsKey("zip"));
    assertTrue(map.containsKey("fmt"));
    assertTrue(map.containsKey("sub_keys"));
    assertTrue(map.containsKey("sub_names"));
    assertFalse(map.containsKey("sub_lnames"));
    int namesSize = map.get("sub_names").split(DELIM).length;
    int keysSize = map.get("sub_keys").split(DELIM).length;
    assertEquals("Expect 3 states in dummy data for Canada.", 3, namesSize);
    assertEquals(namesSize, keysSize);
  }

  @Test public void testFetchingOneKeyManyTimes() {
    LookupKey key = new LookupKey.Builder(CALIFORNIA_KEY).build();

    MockAsyncRequest fakeRequestApi = new MockAsyncRequest("{"
        + "\"id\":\"data/US/CA\","
        + "\"key\":\"CA\","
        + "\"name\":\"California\","
        + "\"lang\":\"en\","
        + "\"zip\":\"9[0-5]|96[01]\","
        + "\"zipex\":\"90000,96199\"}");
    CacheData cache = new CacheData(fakeRequestApi);

    List<DataLoadListener> mockListeners = new ArrayList<DataLoadListener>();
    // Add some listeners before the response is in the cache.
    for (int i = 0; i < 5; ++i) {
      DataLoadListener mockListener = Mockito.mock(DataLoadListener.class);
      mockListeners.add(mockListener);
      cache.fetchDynamicData(key, null, mockListener);
      // Anything added before the result is available should have begun loading.
      Mockito.verify(mockListener).dataLoadingBegin();
      Mockito.verify(mockListener, Mockito.never()).dataLoadingEnd();
    }
    // Check the cache state before and after the response arrives.
    assertFalse("data for key '" + key + "' should not be cached",
        cache.containsKey(key.toString()));
    // Force response to be received here...
    fakeRequestApi.receiveResponse();
    assertTrue("data for key '" + key + "' should be cached", cache.containsKey(key.toString()));
    // Add more listeners after the response is in the cache.
    for (int i = 0; i < 5; ++i) {
      DataLoadListener mockListener = Mockito.mock(DataLoadListener.class);
      mockListeners.add(mockListener);
      cache.fetchDynamicData(key, null, mockListener);
    }
    // Check that all our listener methods were called correctly.
    for (DataLoadListener mockListener : mockListeners) {
      assertListenerWasCalled(mockListener);
    }
  }

  @Test public void testRecursiveFetchFromCallback() {
    final LookupKey key = new LookupKey.Builder(CANADA_KEY).build();

    final MockAsyncRequest fakeRequestApi = new MockAsyncRequest("{"
        + "\"id\":\"data/CA\","
        + "\"key\":\"CA\","
        + "\"name\":\"CANADA\","
        + "}");
    final CacheData cache = new CacheData(fakeRequestApi);

    final DataLoadListener mockListener = Mockito.mock(DataLoadListener.class);
    cache.fetchDynamicData(key, null, new DataLoadListener() {
      @Override
      public void dataLoadingBegin() {
      }

      @Override
      public void dataLoadingEnd() {
        cache.fetchDynamicData(key, null, mockListener);
      }
    });
    fakeRequestApi.receiveResponse();
    assertListenerWasCalled(mockListener);
  }

  @Test public void testInvalidKey() {
    LookupKey key = new LookupKey.Builder(INVALID_KEY).build();

    MockAsyncRequest fakeRequestApi = new MockAsyncRequest("{}");
    CacheData cache = new CacheData(fakeRequestApi);

    DataLoadListener mockListener = Mockito.mock(DataLoadListener.class);
    cache.fetchDynamicData(key, null, mockListener);
    // Check the cache state before and after the response arrives.
    assertFalse("data for key '" + key + "' should not be cached",
        cache.containsKey(key.toString()));
    // Force response to be received here...
    fakeRequestApi.receiveResponse();
    assertFalse("data for key '" + key + "' should not be cached",
        cache.containsKey(key.toString()));
    // Even without a response, the listener was called correctly.
    assertListenerWasCalled(mockListener);
  }

  @Test public void testTimeoutKey() {
    LookupKey key = new LookupKey.Builder(CANADA_KEY).build();

    MockAsyncRequest fakeRequestApi = new MockAsyncRequest("{}");
    CacheData cache = new CacheData(fakeRequestApi);

    DataLoadListener mockListener = Mockito.mock(DataLoadListener.class);
    cache.fetchDynamicData(key, null, mockListener);
    // Check the cache state before and after the response arrives.
    assertFalse("data for key '" + key + "' should not be cached",
        cache.containsKey(key.toString()));
    // Force timeout here...
    fakeRequestApi.timeoutWith();
    assertFalse("data for key '" + key + "' should not be cached",
        cache.containsKey(key.toString()));
    // Even without a response, the listener was called correctly.
    assertListenerWasCalled(mockListener);
  }

  @Test public void testSetUrl() {
    LookupKey key = new LookupKey.Builder(CANADA_KEY).build();
    String url = "http://some-random-url";

    AsyncRequestApi mockRequestApi = Mockito.mock(AsyncRequestApi.class);
    CacheData cache = new CacheData(mockRequestApi);
    cache.setUrl(url);
    cache.fetchDynamicData(key, null, null);
    Mockito.verify(mockRequestApi)
        .requestObject(Mockito.startsWith(url), Mockito.<AsyncCallback>any(), Mockito.anyInt());
  }
}
