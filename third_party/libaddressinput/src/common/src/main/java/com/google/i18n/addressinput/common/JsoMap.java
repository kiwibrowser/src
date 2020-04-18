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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.util.ArrayList;
import java.util.Iterator;

/**
 * Compatibility methods on top of the JSON data.
 */
public final class JsoMap extends JSONObject {
  /**
   * Construct a JsoMap object given some json text. This method directly evaluates the String
   * that you pass in; no error or safety checking is performed, so be very careful about the
   * source of your data.
   *
   * @param json JSON text describing an address format
   * @return a JsoMap object made from the supplied JSON.
   */
  public static JsoMap buildJsoMap(String json) throws JSONException {
    return new JsoMap(new JSONTokener(json));
  }

  /**
   * Construct an empty JsoMap.
   *
   * @return the empty object.
   */
  static JsoMap createEmptyJsoMap() {
    return new JsoMap();
  }

  /**
   * constructor.
   */
  protected JsoMap() {
  }

  private JsoMap(JSONTokener readFrom) throws JSONException {
    super(readFrom);
  }

  private JsoMap(JSONObject copyFrom, String[] names) throws JSONException {
    super(copyFrom, names);
  }

  /**
   * Remove the specified key.
   *
   * @param key key name.
   */
  void delKey(String key) {
    super.remove(key);
  }

  /**
   * Retrieve the string value for specified key.
   *
   * @param key key name.
   * @return string value.
   * @throws ClassCastException, IllegalArgumentException.
   */
  @Override
  public String get(String key) {
    try {
      Object o = super.get(key);
      if (o instanceof String) {
        return (String) o;
      } else if (o instanceof Integer) {
        throw new IllegalArgumentException();
      } else {
        throw new ClassCastException();
      }
    } catch (JSONException e) {
      return null;
    }
  }

  /**
   * Access JSONObject.get(String) which is shadowed by JsoMap.get(String).
   *
   * @param name A key string.
   * @return The object associated with the key.
   * @throws JSONException if the key is not found.
   */
  public Object getObject(String name) throws JSONException {
    return super.get(name);
  }

  /**
   * Retrieves the integer value for specified key.
   *
   * @return integer value or -1 if value is undefined.
   */
  @Override
  public int getInt(String key) {
    try {
      Object o = super.get(key);
      if (o instanceof Integer) {
        return ((Integer) o).intValue();
      } else {
        throw new RuntimeException("Something other than an int was returned");
      }
    } catch (JSONException e) {
      return -1;
    }
  }

  /**
   * Collect all the keys and return as a JSONArray.
   *
   * @return A JSONArray that contains all the keys.
   */
  JSONArray getKeys() {
    // names() returns null if the array was empty!
    JSONArray names = super.names();
    return names != null ? names : new JSONArray();
  }

  /**
   * Retrieve the JsoMap object for specified key.
   *
   * @param key key name.
   * @return JsoMap object.
   * @throws ClassCastException, IllegalArgumentException.
   */
  @SuppressWarnings("unchecked")
  // JSONObject.keys() has no type information.
  JsoMap getObj(String key) throws ClassCastException, IllegalArgumentException {
    try {
      Object o = super.get(key);
      if (o instanceof JSONObject) {
        JSONObject value = (JSONObject) o;
        ArrayList<String> keys = new ArrayList<String>(value.length());
        for (Iterator<String> it = value.keys(); it.hasNext(); ) {
          keys.add(it.next());
        }
        String[] names = new String[keys.size()];
        return new JsoMap(value, keys.toArray(names));
      } else if (o instanceof Integer) {
        throw new IllegalArgumentException();
      } else {
        throw new ClassCastException();
      }
    } catch (JSONException e) {
      return null;
    }
  }

  /**
   * Check if the object has specified key.
   *
   * @param key The key name to be checked.
   * @return true if key can be found.
   */
  boolean containsKey(String key) {
    return super.has(key);
  }

  /**
   * Merge those keys not found in this object from specified object.
   *
   * @param obj The other object to be merged.
   */
  void mergeData(JsoMap obj) {
    if (obj == null) {
      return;
    }

    JSONArray names = obj.names();
    if (names == null) {
      return;
    }

    for (int i = 0; i < names.length(); i++) {
      try {
        String name = names.getString(i);
        try {
          if (!super.has(name)) {
            super.put(name, obj.getObject(name));
          }
        } catch (JSONException e) {
          throw new RuntimeException(e);
        }
      } catch (JSONException e) {
        // Ignored.
      }
    }
  }

  /**
   * Save a string to string mapping into this map.
   *
   * @param key   the string key.
   * @param value the String value.
   */
  void put(String key, String value) {
    try {
      super.put(key, value);
    } catch (JSONException e) {
      throw new RuntimeException(e);
    }
  }

  /**
   * Save a string to integer mapping into this map.
   *
   * @param key   the string key.
   * @param value the integer value.
   */
  void putInt(String key, int value) {
    try {
      super.put(key, value);
    } catch (JSONException e) {
      throw new RuntimeException(e);
    }
  }

  /**
   * Save a string to JSONObject mapping into this map.
   *
   * @param key   the string key.
   * @param value a JSONObject as value.
   */
  void putObj(String key, JSONObject value) {
    try {
      super.put(key, value);
    } catch (JSONException e) {
      throw new RuntimeException(e);
    }
  }

  String string() throws ClassCastException, IllegalArgumentException {
    StringBuilder sb = new StringBuilder("JsoMap[\n");
    JSONArray keys = getKeys();
    for (int i = 0; i < keys.length(); i++) {
      String key;
      try {
        key = keys.getString(i);
      } catch (JSONException e) {
        throw new RuntimeException(e);
      }
      sb.append('(').append(key).append(':').append(get(key)).append(")\n");
    }
    sb.append(']');
    return sb.toString();
  }

  String map() throws ClassCastException, IllegalArgumentException {
    StringBuilder sb = new StringBuilder("JsoMap[\n");
    JSONArray keys = getKeys();
    for (int i = 0; i < keys.length(); i++) {
      String key;
      try {
        key = keys.getString(i);
      } catch (JSONException e) {
        throw new RuntimeException(e);
      }
      sb.append('(').append(key).append(':').append(getObj(key).string()).append(")\n");
    }
    sb.append(']');
    return sb.toString();
  }
}
