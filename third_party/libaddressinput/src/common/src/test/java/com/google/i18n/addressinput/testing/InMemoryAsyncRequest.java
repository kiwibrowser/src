/*
 * Copyright (C) 2015 Google Inc.
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

package com.google.i18n.addressinput.testing;

import com.google.i18n.addressinput.common.AsyncRequestApi;
import com.google.i18n.addressinput.common.JsoMap;

import org.json.JSONException;

import java.util.HashMap;
import java.util.Map;

/**
 * Fake implementation of AsyncRequestApi which loads data synchronously and immediately.
 * Can be used manually with custom test data or with a snapshot of real data found in
 * {@link AddressDataMapLoader#TEST_COUNTRY_DATA}.
 */
public class InMemoryAsyncRequest implements AsyncRequestApi {
  private final Map<String, JsoMap> responseMap = new HashMap<String, JsoMap>();

  public InMemoryAsyncRequest(String urlPrefix, Map<String, String> keyToJsonMap) {
    try {
      for (Map.Entry<String, String> e : keyToJsonMap.entrySet()) {
        responseMap.put(urlPrefix + "/" + e.getKey(), JsoMap.buildJsoMap(e.getValue()));
      }
    } catch (JSONException e) {
      throw new AssertionError("Invalid test JSON data: " + keyToJsonMap, e);
    }
  }

  @Override
  public void requestObject(String url, AsyncCallback callback, int timeoutMillis) {
    if (responseMap.containsKey(url)) {
      callback.onSuccess(responseMap.get(url));
    } else {
      callback.onFailure();
    }
  }
}
