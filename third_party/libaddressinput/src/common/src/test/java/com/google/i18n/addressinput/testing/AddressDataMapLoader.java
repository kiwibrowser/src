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

package com.google.i18n.addressinput.testing;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Helper class to load JSON data for testing.
 */
public class AddressDataMapLoader {
  private static final String DATA_PATH = "countryinfo.txt";

  private AddressDataMapLoader() {
  }

  public static final Map<String, String> TEST_COUNTRY_DATA = loadImmutableTestDataMap();

  private static Map<String, String> loadImmutableTestDataMap() {
    HashMap<String, String> map = new HashMap<String, String>();
    try {
      BufferedReader br = new BufferedReader(new InputStreamReader(
          AddressDataMapLoader.class.getResourceAsStream(DATA_PATH), "UTF-8"));
      String line;
      while (null != (line = br.readLine())) {
        line = line.trim();
        if (line.length() == 0 || line.charAt(0) == '#') {
          continue;
        }
        int x = line.indexOf('=');
        map.put(line.substring(0, x), line.substring(x + 1));
      }
    } catch (IOException e) {
      System.err.println("unable to create map: " + e.getMessage());
    }
    return Collections.unmodifiableMap(map);
  }
}
