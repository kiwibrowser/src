/*
 * Copyright (c) 2001-2003 World Wide Web Consortium,
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

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Logger;

import javax.xml.parsers.DocumentBuilder;

import junit.framework.TestCase;

import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;


public class JUnitTestCaseAdapter extends TestCase implements DOMTestFramework {

  private DOMTestCase test;
  
  
  private static DOMTestDocumentBuilderFactory defaultFactory = null; 

  public JUnitTestCaseAdapter(DOMTestCase test) {
    super(test.getTargetURI());
    test.setFramework(this);
    this.test = test;
  }
//BEGIN android-added
  public JUnitTestCaseAdapter() {
      
  }
  
  private String errorMessage = null;
  private boolean failed = false;
  
  @Override
    public void setName(String name) {
        super.setName(name);
        if (test == null) {
            try {
                URI uri = new URI(name);
                String path = uri.getPath();
                path = path.replaceAll("/", ".");
                Class<?> clazz = null;
                int pos = path.indexOf('.');
                while (pos != -1) {
                    try {
                        clazz = Class.forName("org.w3c.domts." + path);
                        break;
                    } catch (ClassNotFoundException e) {
                        // do nothing
                    }
                    path = path.substring(pos + 1);
                }
                if (clazz == null) {
                    errorMessage = "class not found for test: " + name;
                    failed = true;
                    return;
                }

                if (defaultFactory == null) {
                    defaultFactory = new JAXPDOMTestDocumentBuilderFactory(null,
                            JAXPDOMTestDocumentBuilderFactory.getConfiguration1());
                }

                Constructor<?> constructor = clazz.getConstructor(new Class<?>[] {
                    DOMTestDocumentBuilderFactory.class
                });

                test = (DOMTestCase)constructor.newInstance(new Object[] {
                    defaultFactory
                });
                test.setFramework(this);
                
            } catch (URISyntaxException e) {
                failed = true;
                errorMessage = e.getMessage();
                if (errorMessage == null) {
                    errorMessage = "" + e.toString();
                }
            } catch (IllegalAccessException e) {
                failed = true;
                errorMessage = e.getMessage();
                if (errorMessage == null) {
                    errorMessage = "" + e.toString();
                }
            } catch (InstantiationException e) {
                failed = true;
                errorMessage = e.getMessage();
                if (errorMessage == null) {
                    errorMessage = "" + e.toString();
                }
            } catch (DOMTestIncompatibleException e) {
                failed = true;
                errorMessage = e.getMessage();
                if (errorMessage == null) {
                    errorMessage = "" + e.toString();
                }
            } catch (SecurityException e) {
                failed = true;
                errorMessage = e.getMessage();
                if (errorMessage == null) {
                    errorMessage = "" + e.toString();
                }
            } catch (NoSuchMethodException e) {
                failed = true;
                errorMessage = e.getMessage();
                if (errorMessage == null) {
                    errorMessage = "" + e.toString();
                }
            } catch (IllegalArgumentException e) {
                failed = true;
                errorMessage = e.getMessage();
                if (errorMessage == null) {
                    errorMessage = "" + e.toString();
                }
            } catch (InvocationTargetException e) {
                failed = true;
                Throwable t = e.getCause();
                if (t != null) {
                    errorMessage = t.getMessage();
                    if (errorMessage == null) {
                        errorMessage = "" + t.toString();
                    }
                } else {
                    errorMessage = e.getMessage();
                    if (errorMessage == null) {
                        errorMessage = "" + e.toString();
                    }
                }
            }
        }
    }
//END android-added
  protected void runTest() throws Throwable {
      //BEGIN android-added
      if (failed) {
          if (errorMessage != null) {
              fail(errorMessage);
          } else {
              fail("init failed");
          }
      }
      //END android-added
    test.runTest();
    int mutationCount = test.getMutationCount();
    if (mutationCount != 0) {
    	fail("Document loaded with willBeModified='false' was modified in course of test.");
    }
  }

  public boolean hasFeature(DocumentBuilder docBuilder,
        String feature,
        String version)  {
     return docBuilder.getDOMImplementation().hasFeature(feature,version);
  }

  public void wait(int millisecond) {
  }

  public void fail(DOMTestCase test, String assertID) {
  	fail(assertID);
  }
  
  public void assertTrue(DOMTestCase test, String assertID, boolean actual) {
    assertTrue(assertID,actual);
  }

  public void assertFalse(DOMTestCase test, String assertID, boolean actual) {
    if(actual) {
      assertEquals(assertID,String.valueOf(false), String.valueOf(actual));
    }
  }

  public void assertNull(DOMTestCase test, String assertID, Object actual) {
    assertNull(assertID,actual);
  }

  public void assertNotNull(DOMTestCase test, String assertID, Object actual) {
    assertNotNull(assertID,actual);
  }

  public void assertSame(DOMTestCase test, String assertID, Object expected, Object actual) {
    boolean same = (expected == actual);
    //
    //   if the not the same but both are not null
    //      might still really be the same
    //
    if(!same) {
      if(expected == null || actual == null ||
         !(expected instanceof Node) || !(actual instanceof Node)) {
        assertEquals(assertID,expected,actual);
      }
      else {
        //
        //  could do something with isSame
        //
        assertEquals(assertID,expected,actual);
      }
    }
  }

  public void assertInstanceOf(DOMTestCase test, String assertID, Object obj, Class cls) {
    assertTrue(assertID,cls.isInstance(obj));
  }

  public void assertSize(DOMTestCase test, String assertID, int expectedSize, NodeList collection) {
    assertEquals(assertID,expectedSize, collection.getLength());
  }

  public void assertSize(DOMTestCase test, String assertID, int expectedSize, NamedNodeMap collection) {
    assertEquals(assertID, expectedSize, collection.getLength());
  }

  public void assertSize(DOMTestCase test, String assertID, int expectedSize, Collection collection) {
    assertEquals(assertID, expectedSize, collection.size());
  }

  public void assertEqualsIgnoreCase(DOMTestCase test, String assertID, String expected, String actual) {
  	if (!expected.equalsIgnoreCase(actual)) {
  		assertEquals(assertID,expected, actual);
  	}
  }

  public void assertEqualsIgnoreCase(DOMTestCase test, String assertID, Collection expected, Collection actual) {
    int size = expected.size();
    assertNotNull(assertID,expected);
    assertNotNull(assertID,actual);
    assertEquals(assertID,size, actual.size());
    boolean equals = (expected != null && actual != null && size == actual.size());
    if(equals) {
      List expectedArray = new ArrayList(expected);
      String expectedString;
      String actualString;
      Iterator actualIter = actual.iterator();
      Iterator expectedIter;
      while(actualIter.hasNext() && equals) {
        actualString = (String) actualIter.next();
        expectedIter = expectedArray.iterator();
        equals = false;
        while(expectedIter.hasNext() && !equals) {
          expectedString = (String) expectedIter.next();
          if(actualString.equalsIgnoreCase(expectedString)) {
            equals = true;
            expectedArray.remove(expectedString);
          }
        }
      }
    }
    assertTrue(assertID,equals);
  }

  public void assertEqualsIgnoreCase(DOMTestCase test, String assertID, List expected, List actual) {
    int size = expected.size();
    assertNotNull(assertID,expected);
    assertNotNull(assertID,actual);
    assertEquals(assertID,size, actual.size());
    boolean equals = (expected != null && actual != null && size == actual.size());
    if(equals) {
      String expectedString;
      String actualString;
      for(int i = 0; i < size; i++) {
        expectedString = (String) expected.get(i);
        actualString = (String) actual.get(i);
        if(!expectedString.equalsIgnoreCase(actualString)) {
          assertEquals(assertID,expectedString,actualString);
          break;
        }
      }
    }
  }

  public void assertEquals(DOMTestCase test, String assertID, String expected, String actual) {
    assertEquals(assertID,expected,actual);
  }

  public void assertEquals(DOMTestCase test, String assertID, int expected, int actual) {
    assertEquals(assertID,expected,actual);
  }

  public void assertEquals(DOMTestCase test, String assertID, boolean expected, boolean actual) {
    assertEquals(assertID,expected,actual);
  }

  public void assertEquals(DOMTestCase test, String assertID, double expected, double actual) {
      assertEquals(assertID, expected, actual, 0.0);
  }

  public void assertEquals(DOMTestCase test, String assertID, Collection expected, Collection actual) {
    int size = expected.size();
    assertNotNull(assertID,expected);
    assertNotNull(assertID,actual);
    assertEquals(assertID,size, actual.size());
    boolean equals = (expected != null && actual != null && size == actual.size());
    if(equals) {
      List expectedArray = new ArrayList(expected);
      Object expectedObj;
      Object actualObj;
      Iterator actualIter = actual.iterator();
      Iterator expectedIter;
      while(actualIter.hasNext() && equals) {
        actualObj = actualIter.next();
        expectedIter = expectedArray.iterator();
        equals = false;
        while(expectedIter.hasNext() && !equals) {
          expectedObj = expectedIter.next();
          if(expectedObj == actualObj || expectedObj.equals(actualObj)) {
            equals = true;
            expectedArray.remove(expectedObj);
          }
        }
      }
    }
    assertTrue(assertID,equals);
  }

  public void assertNotEqualsIgnoreCase(DOMTestCase test, String assertID, String expected, String actual) {
    if(expected.equalsIgnoreCase(actual)) {
      assertTrue(assertID, !expected.equalsIgnoreCase(actual));
    }
  }

  public void assertNotEquals(DOMTestCase test, String assertID, String expected, String actual) {
    assertTrue(assertID, !expected.equals(actual));
  }

  public void assertNotEquals(DOMTestCase test, String assertID, int expected, int actual) {
    assertTrue(assertID,expected !=actual);
  }

  public void assertNotEquals(DOMTestCase test, String assertID, boolean expected, boolean actual) {
    assertTrue(assertID,expected !=actual);
  }


  public void assertNotEquals(DOMTestCase test, String assertID, double expected, double actual) {
    if(expected == actual) {
      assertTrue(assertID,expected != actual);
    }
  }


  public boolean same(Object expected, Object actual) {
    boolean equals = (expected == actual);
    if(!equals && expected != null && expected instanceof Node &&
      actual != null && actual instanceof Node) {
      //
      //  can use Node.isSame eventually
    }
    return equals;
  }

  public boolean equalsIgnoreCase(String expected, String actual) {
    return expected.equalsIgnoreCase(actual);
  }

  public boolean equalsIgnoreCase(Collection expected, Collection actual) {
    int size = expected.size();
    boolean equals = (expected != null && actual != null && size == actual.size());
    if(equals) {
      List expectedArray = new ArrayList(expected);
      String expectedString;
      String actualString;
      Iterator actualIter = actual.iterator();
      Iterator expectedIter;
      while(actualIter.hasNext() && equals) {
        actualString = (String) actualIter.next();
        expectedIter = expectedArray.iterator();
        equals = false;
        while(expectedIter.hasNext() && !equals) {
          expectedString = (String) expectedIter.next();
          if(actualString.equalsIgnoreCase(expectedString)) {
            equals = true;
            expectedArray.remove(expectedString);
          }
        }
      }
    }
    return equals;
  }

  public boolean equalsIgnoreCase(List expected, List actual) {
    int size = expected.size();
    boolean equals = (expected != null && actual != null && size == actual.size());
    if(equals) {
      String expectedString;
      String actualString;
      for(int i = 0; i < size; i++) {
        expectedString = (String) expected.get(i);
        actualString = (String) actual.get(i);
        if(!expectedString.equalsIgnoreCase(actualString)) {
          equals = false;
          break;
        }
      }
    }
    return equals;
  }

  public boolean equals(String expected, String actual) {
    return expected.equals(actual);
  }

  public boolean equals(int expected, int actual) {
    return expected == actual;
  }

  public boolean equals(boolean expected, boolean actual) {
    return expected == actual;
  }

  public boolean equals(double expected, double actual) {
    return expected == actual;
  }

  public boolean equals(Collection expected, Collection actual) {
    int size = expected.size();
    boolean equals = (expected != null && actual != null && size == actual.size());
    if(equals) {
      List expectedArray = new ArrayList(expected);
      Object expectedObj;
      Object actualObj;
      Iterator actualIter = actual.iterator();
      Iterator expectedIter;
      while(actualIter.hasNext() && equals) {
        actualObj = actualIter.next();
        expectedIter = expectedArray.iterator();
        equals = false;
        while(expectedIter.hasNext() && !equals) {
          expectedObj = expectedIter.next();
          if(expectedObj != actualObj && expectedObj.equals(actualObj)) {
            equals = true;
            expectedArray.remove(expectedObj);
          }
        }
      }
    }
    return equals;
  }

  public boolean equals(List expected, List actual) {
    int size = expected.size();
    boolean equals = (expected != null && actual != null && size == actual.size());
    if(equals) {
      Object expectedObj;
      Object actualObj;
      for(int i = 0; i < size; i++) {
        expectedObj = expected.get(i);
        actualObj = actual.get(i);
        if(!expectedObj.equals(actualObj)) {
          equals = false;
          break;
        }
      }
    }
    return equals;
  }

  public int size(Collection collection) {
    return collection.size();
  }

  public int size(NamedNodeMap collection) {
    return collection.getLength();
  }

  public int size(NodeList collection) {
    return collection.getLength();
  }


}