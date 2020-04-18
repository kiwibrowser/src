/*
 * Copyright (c) 2001-2004 World Wide Web Consortium,
 * (Massachusetts Institute of Technology, Institut National de
 * Recherche en Informatique et en Automatique, Keio University). All
 * Rights Reserved. This program is distributed under the W3C's Software
 * Intellectual Property License. This program is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.
 * See W3C License http://www.w3.org/Consortium/Legal/ for more details.
 */

package org.w3c.domts;

import java.util.Collection;
import java.util.List;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.NodeList;

/**
 *    This class provides access to DOMTestCase methods (like
 *       assertEquals) for inner classes
 */
public class DOMTestInnerClass {
  private final DOMTestCase test;

  public DOMTestInnerClass(DOMTestCase test) {
    this.test = test;
  }

  public void wait(int millisecond) {
    test.wait(millisecond);
  }

  public void assertTrue(String assertID, boolean actual) {
    test.assertTrue(assertID, actual);
  }

  public void assertFalse(String assertID, boolean actual) {
    test.assertFalse(assertID, actual);
  }

  public void assertNull(String assertID, Object actual) {
    test.assertNull(assertID, actual);
  }

  public void assertNotNull(String assertID, Object actual) {
    test.assertNotNull(assertID, actual);
  }

  public void assertSame(String assertID, Object expected, Object actual) {
    test.assertSame(assertID, expected, actual);
  }

  public void assertInstanceOf(String assertID, Class cls, Object obj) {
    test.assertInstanceOf(assertID, cls, obj);
  }

  public void assertSize(String assertID, int expectedSize, NodeList collection) {
    test.assertSize(assertID, expectedSize, collection);
  }

  public void assertSize(String assertID, int expectedSize,
                         NamedNodeMap collection) {
    test.assertSize(assertID, expectedSize, collection);
  }

  public void assertSize(String assertID, int expectedSize,
                         Collection collection) {
    test.assertSize(assertID, expectedSize, collection);
  }

  public void assertEqualsIgnoreCase(String assertID, String expected,
                                     String actual) {
    test.assertEqualsIgnoreCase(assertID, expected, actual);
  }

  public void assertEqualsIgnoreCase(String assertID, Collection expected,
                                     Collection actual) {
    test.assertEqualsIgnoreCase(assertID, expected, actual);
  }

  public void assertEqualsIgnoreCase(String assertID, List expected,
                                     List actual) {
    test.assertEqualsIgnoreCase(assertID, expected, actual);
  }

  public void assertEquals(String assertID, String expected, String actual) {
    test.assertEquals(assertID, expected, actual);
  }

  public void assertEquals(String assertID, int expected, int actual) {
    test.assertEquals(assertID, expected, actual);
  }

  public void assertEquals(String assertID, double expected, double actual) {
    test.assertEquals(assertID, expected, actual);
  }

  public void assertEquals(String assertID, boolean expected, boolean actual) {
    test.assertEquals(assertID, expected, actual);
  }

  public void assertEquals(String assertID, Collection expected,
                           NodeList actual) {
    test.assertEquals(assertID, expected, actual);
  }

  public void assertEquals(String assertID, Collection expected,
                           Collection actual) {
    test.assertEquals(assertID, expected, actual);
  }

  public void assertNotEqualsIgnoreCase(String assertID, String expected,
                                        String actual) {
    test.assertNotEqualsIgnoreCase(assertID, expected, actual);
  }

  public void assertNotEquals(String assertID, String expected, String actual) {
    test.assertNotEquals(assertID, expected, actual);
  }

  public void assertNotEquals(String assertID, int expected, int actual) {
    test.assertNotEquals(assertID, expected, actual);
  }

  public void assertNotEquals(String assertID, double expected, double actual) {
    test.assertNotEquals(assertID, expected, actual);
  }

  public void assertURIEquals(String assertID, String scheme, String path,
                              String host, String file, String name,
                              String query, String fragment, Boolean isAbsolute,
                              String actual) {
    test.assertURIEquals(assertID, scheme, path, host, file, name, query,
                         fragment, isAbsolute, actual);
  }

  public boolean same(Object expected, Object actual) {
    return test.same(expected, actual);
  }

  public boolean equalsIgnoreCase(String expected, String actual) {
    return test.equalsIgnoreCase(expected, actual);
  }

  public boolean equalsIgnoreCase(Collection expected, Collection actual) {
    return test.equalsIgnoreCase(expected, actual);
  }

  public boolean equalsIgnoreCase(List expected, List actual) {
    return test.equalsIgnoreCase(expected, actual);
  }

  public boolean equals(String expected, String actual) {
    return test.equals(expected, actual);
  }

  public boolean equals(int expected, int actual) {
    return test.equals(expected, actual);
  }

  public boolean equals(double expected, double actual) {
    return test.equals(expected, actual);
  }

  public boolean equals(Collection expected, Collection actual) {
    return test.equals(expected, actual);
  }

  public boolean equals(List expected, List actual) {
    return test.equals(expected, actual);
  }

  public int size(Collection collection) {
    return test.size(collection);
  }

  public int size(NamedNodeMap collection) {
    return test.size(collection);
  }

  public int size(NodeList collection) {
    return test.size(collection);
  }

  public DOMImplementation getImplementation() {
    return test.getImplementation();
  }

}
