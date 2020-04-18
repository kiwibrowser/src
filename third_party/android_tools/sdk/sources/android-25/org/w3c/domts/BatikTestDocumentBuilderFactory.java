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

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.Document;

/**
 *   This class implements the generic parser and configuation
 *   abstract class for the DOM implementation of Batik.
 *
 *   @author Curt Arnold
 */
public class BatikTestDocumentBuilderFactory
    extends DOMTestDocumentBuilderFactory {

  /**  dom factory.  */
  private Object domFactory;

  /**  xml reader.    */
  private org.xml.sax.XMLReader xmlReader;

  /**  reflective method to create document in Batik.   **/
  private Method createDocument;

  /**  dom implementation from Batik.   **/
  private DOMImplementation domImpl;

  /**
   * Creates a Batik implementation of DOMTestDocumentBuilderFactory.
   * @param settings array of settings, may be null.
   * @throws DOMTestIncompatibleException
   *     If implementation does not support the specified settings
   */
  public BatikTestDocumentBuilderFactory(
      DocumentBuilderSetting[] settings) throws DOMTestIncompatibleException {
    super(settings);
    domImpl = null;

    //
    //   get the JAXP specified SAX parser's class name
    //
    SAXParserFactory saxFactory = SAXParserFactory.newInstance();
    try {
      SAXParser saxParser = saxFactory.newSAXParser();
      xmlReader = saxParser.getXMLReader();
    } catch (Exception ex) {
      throw new DOMTestIncompatibleException(ex, null);
    }
    String xmlReaderClassName = xmlReader.getClass().getName();

    //
    //   can't change settings, so if not the same as
    //      the default SAX parser then throw an exception
    //
    //    for(int i = 0; i < settings.length; i++) {
    //      if(!settings[i].hasSetting(this)) {
    //        TODO
    //        throw new DOMTestIncompatibleException(null,settings[i]);
    //      }
    //    }
    //
    //   try loading Batik reflectively
    //
    try {
      ClassLoader classLoader = ClassLoader.getSystemClassLoader();
      Class domFactoryClass =
          classLoader.loadClass(
          "org.apache.batik.dom.svg.SAXSVGDocumentFactory");

      Constructor domFactoryConstructor =
          domFactoryClass.getConstructor(new Class[] {String.class});
      domFactory =
          domFactoryConstructor.newInstance(
          new Object[] {xmlReaderClassName});
      createDocument =
          domFactoryClass.getMethod(
          "createDocument",
          new Class[] {String.class, java.io.InputStream.class});
    } catch (InvocationTargetException ex) {
      throw new DOMTestIncompatibleException(
          ex.getTargetException(),
          null);
    } catch (Exception ex) {
      throw new DOMTestIncompatibleException(ex, null);
    }
  }

  /**
   *    Create new instance of document builder factory
   *    reflecting specified settings.
   *    @param newSettings new settings
   *    @return New instance
   *    @throws DOMTestIncompatibleException
   *         if settings are not supported by implementation
   */
  public DOMTestDocumentBuilderFactory newInstance(
      DocumentBuilderSetting[] newSettings)
        throws DOMTestIncompatibleException {
    if (newSettings == null) {
      return this;
    }
    DocumentBuilderSetting[] mergedSettings = mergeSettings(newSettings);
    return new BatikTestDocumentBuilderFactory(mergedSettings);
  }

  /**
   *    Loads specified URL.
   *    @param url url to load
   *    @return DOM document
   *    @throws DOMTestLoadException if unable to load document
   */
  public Document load(java.net.URL url) throws DOMTestLoadException {
    try {
      java.io.InputStream stream = url.openStream();
      return (org.w3c.dom.Document) createDocument.invoke(
          domFactory,
          new Object[] {url.toString(), stream});
    } catch (InvocationTargetException ex) {
      ex.printStackTrace();
      throw new DOMTestLoadException(ex.getTargetException());
    } catch (Exception ex) {
      ex.printStackTrace();
      throw new DOMTestLoadException(ex);
    }
  }

  /**
   *     Gets DOMImplementation.
   *     @return DOM implementation, may be null
   */
  public DOMImplementation getDOMImplementation() {
    //
    //   get DOM implementation
    //
    if (domImpl == null) {
      try {
        Class svgDomImplClass =
            ClassLoader.getSystemClassLoader().loadClass(
            "org.apache.batik.dom.svg.SVGDOMImplementation");
        Method getImpl =
            svgDomImplClass.getMethod(
            "getDOMImplementation",
            new Class[0]);
        domImpl =
            (DOMImplementation) getImpl.invoke(null, new Object[0]);
      } catch (Exception ex) {
        return null;
      }
    }
    return domImpl;
  }

  /**
   *   Determines if the implementation supports the specified feature.
   *   @param feature Feature
   *   @param version Version
   *   @return true if implementation supports the feature
   */
  public boolean hasFeature(String feature, String version) {
    return getDOMImplementation().hasFeature(feature, version);
  }

  /**
   *   Adds any specialized extension required by the implementation.
   *   @param testFileName file name from test
   *   @return possibly modified file name
   */
  public String addExtension(String testFileName) {
    return testFileName + ".svg";
  }

  /**
   *   Indicates whether the implementation combines text and cdata nodes.
   *   @return true if coalescing
   */
  public boolean isCoalescing() {
    return false;
  }

  /**
   *   Indicates whether the implementation expands entity references.
   *   @return true if expanding entity references
   */
  public boolean isExpandEntityReferences() {
    return false;
  }

  /**
   *   Indicates whether the implementation ignores
   *       element content whitespace.
   *   @return true if ignoring element content whitespace
   */
  public boolean isIgnoringElementContentWhitespace() {
    return false;
  }

  /**
   *   Indicates whether the implementation is namespace aware.
   *   @return true if namespace aware
   */
  public boolean isNamespaceAware() {
    return true;
  }

  /**
   *   Indicates whether the implementation is validating.
   *   @return true if validating
   */
  public boolean isValidating() {
    return false;
  }

  /**
   * Gets content type.
   * @return content type, "image/svg+xml"
   */
  public String getContentType() {
    return "image/svg+xml";
  }

}
