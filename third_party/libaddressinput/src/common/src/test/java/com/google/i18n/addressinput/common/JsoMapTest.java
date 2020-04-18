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

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

@RunWith(JUnit4.class)
public class JsoMapTest extends TestCase {
  private static final String VALID_JSON = "{\"a\":\"b\",\"c\":1,\"d\":{\"e\":\"f\"}}";
  private static final String INVALID_JSON = "!";

  @Test public void testBuildJsoMap() throws Exception {
    assertNotNull(JsoMap.buildJsoMap(VALID_JSON));

    try {
      JsoMap.buildJsoMap(INVALID_JSON);
      fail("Expected JSONException.");
    } catch (JSONException e) {
      // Expected.
    }
  }

  @Test public void testCreateEmptyJsoMap() throws Exception {
    assertNotNull(JsoMap.createEmptyJsoMap());
  }

  @Test public void testDelKey() throws Exception {
    JsoMap map = JsoMap.buildJsoMap(VALID_JSON);

    assertEquals("b", map.get("a"));
    map.delKey("a");
    assertNull(map.get("a"));

    map.delKey("b");
    map.delKey("c");
    map.delKey("d");
  }

  @Test public void testGet() throws Exception {
    JsoMap map = JsoMap.buildJsoMap(VALID_JSON);
    assertEquals("b", map.get("a"));
    assertNull(map.get("b"));

    try {
      map.get("c");
      fail("Expected IllegalArgumentException.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      map.get("d");
      fail("Expected ClassCastException.");
    } catch (ClassCastException e) {
      // Expected.
    }
  }

  @Test public void testGetInt() throws Exception {
    JsoMap map = JsoMap.buildJsoMap(VALID_JSON);

    try {
      map.getInt("a");
      fail("Expected RuntimeException.");
    } catch (RuntimeException e) {
      // Expected.
    }

    assertEquals(-1, map.getInt("b"));
    assertEquals(1, map.getInt("c"));

    try {
      map.getInt("d");
      fail("Expected RuntimeException.");
    } catch (RuntimeException e) {
      // Expected.
    }
  }

  @Test public void testGetKeys() throws Exception {
    JsoMap map = JsoMap.buildJsoMap(VALID_JSON);
    JSONArray keys = map.getKeys();
    assertNotNull(keys);
    assertEquals(3, keys.length());
    Set<String> keySet = new HashSet<String>(keys.length());
    for (int i = 0; i < keys.length(); i++) {
      keySet.add(keys.getString(i));
    }
    assertEquals(new HashSet<String>(Arrays.asList("a", "c", "d")), keySet);
  }

  @Test public void testGetObj() throws Exception {
    JsoMap map = JsoMap.buildJsoMap(VALID_JSON);

    try {
      map.getObj("a");
      fail("Expected ClassCastException.");
    } catch (ClassCastException e) {
      // Expected.
    }

    assertNull(map.getObj("b"));

    try {
      map.getObj("c");
      fail("Expected IllegalArgumentException.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    JsoMap obj = map.getObj("d");
    assertNotNull(obj);
    assertEquals("f", obj.get("e"));
  }

  @Test public void testContainsKey() throws Exception {
    JsoMap map = JsoMap.buildJsoMap(VALID_JSON);
    assertTrue(map.containsKey("a"));
    assertFalse(map.containsKey("b"));
    assertTrue(map.containsKey("c"));
    assertTrue(map.containsKey("d"));
  }

  @Test public void testMergeData() throws Exception {
    JsoMap mapA = JsoMap.createEmptyJsoMap();
    JsoMap mapB = JsoMap.createEmptyJsoMap();

    mapA.putInt("a", 1);
    mapA.putInt("b", 2);

    mapB.putInt("b", 3);
    mapB.putInt("c", 4);

    mapA.mergeData(mapB);
    assertEquals(1, mapA.getInt("a"));
    assertEquals(2, mapA.getInt("b"));
    assertEquals(4, mapA.getInt("c"));
  }

  @Test public void testPut() throws Exception {
    JsoMap map = JsoMap.createEmptyJsoMap();

    map.put("a", "b");
    assertEquals("b", map.get("a"));

    map.put("a", "c");
    assertEquals("c", map.get("a"));
  }

  @Test public void testPutInt() throws Exception {
    JsoMap map = JsoMap.createEmptyJsoMap();

    map.putInt("a", 1);
    assertEquals(1, map.getInt("a"));

    map.putInt("a", 2);
    assertEquals(2, map.getInt("a"));
  }

  @Test public void testPutObj() throws Exception {
    JsoMap map = JsoMap.createEmptyJsoMap();
    JsoMap obj = JsoMap.createEmptyJsoMap();

    obj.putInt("a", 1);
    map.putObj("b", obj);
    assertEquals(obj.toString(), map.getObj("b").toString());

    obj.putInt("a", 2);
    map.putObj("b", obj);
    assertEquals(obj.toString(), map.getObj("b").toString());
  }

  @Test public void testString() throws Exception {
    JsoMap map = JsoMap.buildJsoMap(VALID_JSON);

    try {
      // This should fail on the integer "c" or the map "d".
      map.string();
      fail("Expected IllegalArgumentException.");
    } catch (IllegalArgumentException e) {
      // Expected.
    } catch (ClassCastException e) {
      // Expected.
    }

    map.delKey("c");
    try {
      // This should fail on the object "d".
      map.string();
      fail("Expected ClassCastException.");
    } catch (ClassCastException e) {
      // Expected.
    }

    map.delKey("d");
    assertEquals("JsoMap[\n(a:b)\n]", map.string());
  }

  @Test public void testMap() throws Exception {
    JsoMap map = JsoMap.buildJsoMap(VALID_JSON);
    try {
      // This should fail on the string "a" or the integer "c".
      map.map();
      fail("Expected ClassCastException.");
    } catch (ClassCastException e) {
      // Expected.
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    map.delKey("a");
    try {
      // This should fail on the integer "c".
      map.map();
      fail("Expected IllegalArgumentException.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    map.delKey("c");
    assertEquals("JsoMap[\n(d:JsoMap[\n(e:f)\n])\n]", map.map());
  }
}
