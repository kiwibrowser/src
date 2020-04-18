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

import java.io.InputStream;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.Document;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

/**
 *   This class implements the generic parser and configuation
 *   abstract class for JAXP supporting parsers.
 */
public class JAXPDOMTestDocumentBuilderFactory
    extends DOMTestDocumentBuilderFactory {

  private DocumentBuilderFactory factory;
  private DocumentBuilder builder;

  /**
   * Creates a JAXP implementation of DOMTestDocumentBuilderFactory.
   * @param factory null for default JAXP provider.  If not null,
   * factory will be mutated in constructor and should be released
   * by calling code upon return.
   * @param settings array of settings, may be null.
   */
  public JAXPDOMTestDocumentBuilderFactory(
      DocumentBuilderFactory baseFactory,
      DocumentBuilderSetting[] settings) throws DOMTestIncompatibleException {
    super(settings);
    if (baseFactory == null) {
      factory = DocumentBuilderFactory.newInstance();
    }
    else {
      factory = baseFactory;
    }
    //
    //    apply settings to selected document builder
    //         may throw exception if incompatible
    if (settings != null) {
      for (int i = 0; i < settings.length; i++) {
        settings[i].applySetting(factory);
      }
    }
    try {
      this.builder = factory.newDocumentBuilder();
    }
    catch (ParserConfigurationException ex) {
      throw new DOMTestIncompatibleException(ex, null);
    }
  }

  protected DOMTestDocumentBuilderFactory createInstance(DocumentBuilderFactory
      newFactory,
      DocumentBuilderSetting[] mergedSettings) throws
      DOMTestIncompatibleException {
    return new JAXPDOMTestDocumentBuilderFactory(newFactory, mergedSettings);
  }

  public DOMTestDocumentBuilderFactory newInstance(DocumentBuilderSetting[]
      newSettings) throws DOMTestIncompatibleException {
    if (newSettings == null) {
      return this;
    }
    DocumentBuilderSetting[] mergedSettings = mergeSettings(newSettings);
    DocumentBuilderFactory newFactory = factory.newInstance();
    return createInstance(newFactory, mergedSettings);
  }

  private class LoadErrorHandler
      implements org.xml.sax.ErrorHandler {
    private SAXException parseException;
    private int errorCount;
    private int warningCount;
    public LoadErrorHandler() {
      parseException = null;
      errorCount = 0;
      warningCount = 0;
    }

    public void error(SAXParseException ex) {
      errorCount++;
      if (parseException == null) {
        parseException = ex;
      }
    }

    public void warning(SAXParseException ex) {
      warningCount++;
    }

    public void fatalError(SAXParseException ex) {
      if (parseException == null) {
        parseException = ex;
      }
    }

    public SAXException getFirstException() {
      return parseException;
    }
  }

  public Document load(java.net.URL url) throws DOMTestLoadException {
    Document doc = null;
    Exception parseException = null;
    try {
      LoadErrorHandler errorHandler = new LoadErrorHandler();
      builder.setErrorHandler(errorHandler);
      InputStream stream = url.openStream();
      doc = builder.parse(stream, url.toString());
      stream.close();
      parseException = errorHandler.getFirstException();
    }
    catch (Exception ex) {
      parseException = ex;
    }
    builder.setErrorHandler(null);
    if (parseException != null) {
      throw new DOMTestLoadException(parseException);
    }
    return doc;
  }

  public DOMImplementation getDOMImplementation() {
    return builder.getDOMImplementation();
  }

  public boolean hasFeature(String feature, String version) {
    return builder.getDOMImplementation().hasFeature(feature, version);
  }

  public boolean isCoalescing() {
    return factory.isCoalescing();
  }

  public boolean isExpandEntityReferences() {
    return factory.isExpandEntityReferences();
  }

  public boolean isIgnoringElementContentWhitespace() {
    return factory.isIgnoringElementContentWhitespace();
  }

  public boolean isNamespaceAware() {
    return factory.isNamespaceAware();
  }

  public boolean isValidating() {
    return factory.isValidating();
  }

  public static DocumentBuilderSetting[] getConfiguration1() {
    return new DocumentBuilderSetting[] {
        DocumentBuilderSetting.notCoalescing,
        DocumentBuilderSetting.notExpandEntityReferences,
        DocumentBuilderSetting.notIgnoringElementContentWhitespace,
        DocumentBuilderSetting.notNamespaceAware,
        DocumentBuilderSetting.notValidating};
  }

  public static DocumentBuilderSetting[] getConfiguration2() {
    return new DocumentBuilderSetting[] {
        DocumentBuilderSetting.notCoalescing,
        DocumentBuilderSetting.expandEntityReferences,
        DocumentBuilderSetting.ignoringElementContentWhitespace,
        DocumentBuilderSetting.namespaceAware,
        DocumentBuilderSetting.validating};

  }

}
