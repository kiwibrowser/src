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

import java.util.Collections;
import java.util.EnumMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Wraps a Map of address property data to provide the AddressVerificationData API.
 */
public final class AddressVerificationData implements DataSource {
  private final Map<String, String> propertiesMap;

  private static final Pattern KEY_VALUES_PATTERN = Pattern.compile("\"([^\"]+)\":\"([^\"]*)\"");

  private static final Pattern SEPARATOR_PATTERN = Pattern.compile("\",\"");

  /**
   * Constructs from a map of address property data.  This keeps a reference to the map.  This
   * does not mutate the map. The map should not be mutated subsequent to this call.
   */
  public AddressVerificationData(Map<String, String> propertiesMap) {
    this.propertiesMap = propertiesMap;
  }

  /**
   * This will not return null.
   */
  @Override
  public AddressVerificationNodeData getDefaultData(String key) {
    // gets country key
    if (key.split("/").length > 1) {
      String[] parts = key.split("/");
      key = parts[0] + "/" + parts[1];
    }

    AddressVerificationNodeData data = get(key);
    if (data == null) {
      throw new RuntimeException("failed to get default data with key " + key);
    }
    return data;
  }

  @Override
  public AddressVerificationNodeData get(String key) {
    String json = propertiesMap.get(key);
    if (json != null && isValidDataKey(key)) {
      return createNodeData(json);
    }
    return null;
  }

  /**
   * Returns a set of the keys for which verification data is provided.  The returned set is
   * immutable.
   */
  Set<String> keys() {
    Set<String> result = new HashSet<String>();
    for (String key : propertiesMap.keySet()) {
      if (isValidDataKey(key)) {
        result.add(key);
      }
    }
    return Collections.unmodifiableSet(result);
  }

  /**
   * Returns whether the key is a "data" key rather than an "examples" key.
   */
  private boolean isValidDataKey(String key) {
    return key.startsWith("data");
  }

  /**
   * Returns the contents of the JSON-format string as a map.
   */
  AddressVerificationNodeData createNodeData(String json) {
    // Remove leading and trailing { and }.
    json = json.substring(1, json.length() - 1);
    Map<AddressDataKey, String> map = new EnumMap<AddressDataKey, String>(AddressDataKey.class);

    // our objects are very simple so we parse manually
    // - no double quotes within strings
    // - no extra spaces
    // can't use split "," since some data has commas in it.
    Matcher sm = SEPARATOR_PATTERN.matcher(json);
    int pos = 0;
    while (pos < json.length()) {
      String pair;
      if (sm.find()) {
        pair = json.substring(pos, sm.start() + 1);
        pos = sm.start() + 2;
      } else {
        pair = json.substring(pos);
        pos = json.length();
      }

      Matcher m = KEY_VALUES_PATTERN.matcher(pair);
      if (m.matches()) {
        String value = m.group(2);

        // Remove escaped backslashes.
        // Java regex doesn't handle a replacement String consisting of
        // a single backslash, and treats a replacement String consisting of
        // two backslashes as two backslashes instead of one.  So there's
        // no way to use regex to replace a match with a single backslash,
        // apparently.
        if (value.length() > 0) {
          char[] linechars = m.group(2).toCharArray();
          int w = 1;
          for (int r = w; r < linechars.length; ++r) {
            char c = linechars[r];
            if (c == '\\' && linechars[w - 1] == '\\') {
              // don't increment w;
              continue;
            }
            linechars[w++] = c;
          }
          value = new String(linechars, 0, w);
        }

        AddressDataKey df = AddressDataKey.get(m.group(1));
        if (df == null) {
          // Skip this data - it isn't used in the Android version.
        } else {
          map.put(df, value);
        }
      } else {
        // This is a runtime data sanity check.  The data should be
        // checked when the data is built.  The JSON data string should
        // be parsable into string pairs using SEP_PAT.
        throw new RuntimeException("could not match '" + pair + "' in '" + json + "'");
      }
    }

    return new AddressVerificationNodeData(map);
  }
}
