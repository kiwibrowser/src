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

/*
  $Log: XalanDOMTestDocumentBuilderFactory.java,v $
  Revision 1.2  2004/03/11 01:44:21  dom-ts-4
  Checkstyle fixes (bug 592)

  Revision 1.1  2003/04/24 05:02:05  dom-ts-4
  Xalan-J support for L3 XPath
  http://www.w3.org/Bugs/Public/show_bug.cgi?id=191

  Revision 1.1  2002/02/03 07:47:51  dom-ts-4
  More missing files

 */

package org.w3c.domts;

import java.lang.reflect.Constructor;

import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;

/**
 *
 *   This class uses Xalan-J to add XPath support
 *       to the current JAXP DOM implementation
 */
public class XalanDOMTestDocumentBuilderFactory
    extends JAXPDOMTestDocumentBuilderFactory {

  /**
   * Creates a JAXP implementation of DOMTestDocumentBuilderFactory.
   * @param factory null for default JAXP provider.  If not null,
   * factory will be mutated in constructor and should be released
   * by calling code upon return.
   * @param settings array of settings, may be null.
   */
  public XalanDOMTestDocumentBuilderFactory(
      DocumentBuilderFactory baseFactory,
      DocumentBuilderSetting[] settings) throws DOMTestIncompatibleException {
    super(baseFactory, settings);
  }

  protected DOMTestDocumentBuilderFactory createInstance(DocumentBuilderFactory
      newFactory,
      DocumentBuilderSetting[] mergedSettings) throws
      DOMTestIncompatibleException {
    return new XalanDOMTestDocumentBuilderFactory(newFactory, mergedSettings);
  }

  /**
   *  Creates XPath evaluator
   *  @param doc DOM document, may not be null
   */
  public Object createXPathEvaluator(Document doc) {
    try {
      Class xpathClass = Class.forName(
          "org.apache.xpath.domapi.XPathEvaluatorImpl");
      Constructor constructor = xpathClass.getConstructor(new Class[] {Document.class});
      return constructor.newInstance(new Object[] {doc});
    }
    catch (Exception ex) {
    }
    return doc;
  }

}
