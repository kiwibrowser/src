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

/*
  $Log: DOM4JTestDocumentBuilderFactory.java,v $
  Revision 1.2  2004/03/11 01:44:21  dom-ts-4
  Checkstyle fixes (bug 592)

  Revision 1.1  2002/02/03 07:47:51  dom-ts-4
  More missing files

 */

package org.w3c.domts;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.Document;
import org.xml.sax.XMLReader;

/**
 *   This class implements the generic parser and configuation
 *   abstract class for JAXP supporting parsers.
 */
public class DOM4JTestDocumentBuilderFactory
    extends DOMTestDocumentBuilderFactory {

  private final Object domFactory;
  private final Object saxReader;
  private final org.xml.sax.XMLReader xmlReader;
  private org.w3c.dom.DOMImplementation domImpl;
  private final Method readMethod;

  /**
   * Creates a JAXP implementation of DOMTestDocumentBuilderFactory.
   * @param factory null for default JAXP provider.  If not null,
   * factory will be mutated in constructor and should be released
   * by calling code upon return.
   * @param XMLReader if null use default XMLReader.  If provided,
   * it may be mutated and should be released by the caller immediately
   * after the constructor.
   * @param settings array of settings, may be null.
   */
  public DOM4JTestDocumentBuilderFactory(DocumentBuilderSetting[] settings) throws
      DOMTestIncompatibleException {
    super(settings);
    try {
      //
      //   The following reflection code is trying to accomplish
      //
      //domFactory = org.dom4j.dom.DOMDocumentFactory.getInstance();
      //domImpl = (DOMImplementation) domFactory;
      //saxReader = new org.dom4j.io.SAXReader(domFactory);
      //xmlReader = saxReader.getXMLReader();

      ClassLoader classLoader = ClassLoader.getSystemClassLoader();
      Class domFactoryClass = classLoader.loadClass(
          "org.dom4j.dom.DOMDocumentFactory");
      Method getInstance = domFactoryClass.getMethod("getInstance", new Class[] {});
      domFactory = getInstance.invoke(null, new Object[] {});
      domImpl = (DOMImplementation) domFactory;
      Class saxReaderClass = classLoader.loadClass("org.dom4j.io.SAXReader");
      Constructor saxReaderConstructor = saxReaderClass.getConstructor(
          new Class[] {classLoader.loadClass("org.dom4j.DocumentFactory")});
      saxReader = saxReaderConstructor.newInstance(new Object[] {domFactory});

      Method getReaderMethod = saxReaderClass.getMethod("getXMLReader",
          new Class[] {});
      xmlReader = (XMLReader) getReaderMethod.invoke(saxReader, new Object[0]);

      readMethod = saxReaderClass.getMethod("read", new Class[] {java.net.URL.class});
    }
    catch (InvocationTargetException ex) {
      throw new DOMTestIncompatibleException(ex.getTargetException(), null);
    }
    catch (Exception ex) {
      throw new DOMTestIncompatibleException(ex, null);
    }
    //
    //   TODO: Process settings
    //
  }

  public DOMTestDocumentBuilderFactory newInstance(DocumentBuilderSetting[]
      newSettings) throws DOMTestIncompatibleException {
    if (newSettings == null) {
      return this;
    }
    DocumentBuilderSetting[] mergedSettings = mergeSettings(newSettings);
    return new DOM4JTestDocumentBuilderFactory(mergedSettings);
  }

  public Document load(java.net.URL url) throws DOMTestLoadException {
    if (url == null) {
      throw new NullPointerException("url");
    }
    if (saxReader == null) {
      throw new NullPointerException("saxReader");
    }
    try {
      return (org.w3c.dom.Document) readMethod.invoke(saxReader,
          new Object[] {url});
    }
    catch (InvocationTargetException ex) {
      ex.getTargetException().printStackTrace();
      throw new DOMTestLoadException(ex.getTargetException());
    }
    catch (Exception ex) {
      ex.printStackTrace();
      throw new DOMTestLoadException(ex);
    }
  }

  public DOMImplementation getDOMImplementation() {
    return domImpl;
  }

  public boolean hasFeature(String feature, String version) {
    return domImpl.hasFeature(feature, version);
  }

  public boolean isCoalescing() {
    return false;
  }

  public boolean isExpandEntityReferences() {
    return false;
  }

  public boolean isIgnoringElementContentWhitespace() {
    return false;
  }

  public boolean isNamespaceAware() {
    return true;
  }

  public boolean isValidating() {
    return false;
  }

}
