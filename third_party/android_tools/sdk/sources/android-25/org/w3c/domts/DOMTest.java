/*
 * Copyright (c) 2001-2003 World Wide Web Consortium, (Massachusetts Institute
 * of Technology, Institut National de Recherche en Informatique et en
 * Automatique, Keio University). All Rights Reserved. This program is
 * distributed under the W3C's Software Intellectual Property License. This
 * program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See W3C License
 * http://www.w3.org/Consortium/Legal/ for more details.
 */


package org.w3c.domts;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.net.MalformedURLException;
import java.net.URL;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.Document;

/**
 * This is an abstract base class for generated DOM tests
 *
 */
public abstract class DOMTest /* wBM: implements EventListener */ {
  private DOMTestDocumentBuilderFactory factory;
  private int mutationCount = 0;

  /**
   * This is the appropriate constructor for tests that make no requirements
   * on the parser configuration.
   *
   * @param factory
   *            must not be null
   */
  public DOMTest(DOMTestDocumentBuilderFactory factory) {
    if (factory == null) {
      throw new NullPointerException("factory");
    }
    this.factory = factory;
  }

  /**
   * This constructor is used by tests that must create a modified document
   * factory to meet requirements on the parser configuration. setFactory
   * should be called within the test's constructor.
   */
  public DOMTest() {
    factory = null;
  }

  /**
   * Should only be called in the constructor of a derived type.
   */
  protected void setFactory(DOMTestDocumentBuilderFactory factory) {
    this.factory = factory;
  }

  public boolean hasFeature(String feature, String version) {
    return factory.hasFeature(feature, version);
  }

  public boolean hasSetting(DocumentBuilderSetting setting) {
    return setting.hasSetting(factory);
  }

  protected DOMTestDocumentBuilderFactory getFactory() {
    return factory;
  }

  public DOMImplementation getImplementation() {
    return factory.getDOMImplementation();
  }

  private URL resolveURI(String baseURI) throws DOMTestLoadException {
    String docURI = factory.addExtension(baseURI);

    URL resolvedURI = null;
    try {
      resolvedURI = new URL(docURI);
      if (resolvedURI.getProtocol() != null) {
        return resolvedURI;
      }
    }
    catch (MalformedURLException ex) {
      //        throw new DOMTestLoadException(ex);
    }
    //
    //   build a URL for a test file in the JAR
    //
    resolvedURI = getClass().getResource("/" + docURI);
    if (resolvedURI == null) {
      //
      //   see if it is an absolute URI
      //
      int firstSlash = docURI.indexOf('/');
      try {
        if (firstSlash == 0
            || (firstSlash >= 1
                && docURI.charAt(firstSlash - 1) == ':')) {
          resolvedURI = new URL(docURI);
        }
        else {
          //
          //  try the files/level?/spec directory
          //
          String filename = getClass().getPackage().getName();
          filename =
              "tests/"
              + filename.substring(14).replace('.', '/')
              + "/files/"
              + docURI;
          resolvedURI = new java.io.File(filename).toURL();
        }
      }
      catch (MalformedURLException ex) {
        throw new DOMTestLoadException(ex);
      }
    }

    if (resolvedURI == null) {
      throw new DOMTestLoadException(
          new java.io.FileNotFoundException(docURI));
    }
    return resolvedURI;
  }

  public String getResourceURI(String href, String scheme, String contentType) throws
      DOMTestLoadException {
    if (scheme == null) {
      throw new NullPointerException("scheme");
    }
    if ("file".equals(scheme)) {
      return resolveURI(href).toString();
    }
    if ("http".equals(scheme)) {
      StringBuffer httpURL = new StringBuffer(
          System.getProperty("org.w3c.domts.httpbase",
                             "http://localhost:8080/webdav/"));
      httpURL.append(href);
      if ("application/pdf".equals(contentType)) {
        httpURL.append(".pdf");
      }
      else {
        httpURL.append(".xml");
      }
      return httpURL.toString();
    }
    throw new DOMTestLoadException(new Exception("Unrecognized URI scheme " +
                                                 scheme));
  }

  public String createTempURI(String scheme) throws DOMTestLoadException {
    if (scheme == null) {
      throw new NullPointerException("scheme");
    }
    if ("file".equals(scheme)) {
      try {
        File tempFile = File.createTempFile("domts", ".xml");
        try {
          //
          //   if available use JDK 1.4's File.toURI().toString()
          //
          Method method = File.class.getMethod("toURI", (Class<?>) null);
          Object uri = method.invoke(tempFile, (Class<?>) null);
          return uri.toString();
        }
        catch (NoSuchMethodException ex) {
          //
          //   File.toURL is not as robust
          //
          URL url = tempFile.toURL();
          return url.toString();
        }
      }
      catch (Exception ex) {
        throw new DOMTestLoadException(ex);
      }
    }
    if ("http".equals(scheme)) {
      String httpBase = System.getProperty("org.w3c.domts.httpbase",
                                           "http://localhost:8080/webdav/");
      java.lang.StringBuffer buf = new StringBuffer(httpBase);
      if (!httpBase.endsWith("/")) {
          buf.append("/");
      }
      buf.append("tmp");
      buf.append( (new java.util.Random()).nextInt(Integer.MAX_VALUE));
      buf.append(".xml");
      return buf.toString();
    }
    throw new DOMTestLoadException(new Exception("Unrecognized URI scheme " +
                                                 scheme));
  }

  public Document load(String docURI, boolean willBeModified) throws
      DOMTestLoadException {
    Document doc = factory.load(resolveURI(docURI));
    //
    //   if will be modified is false and doc is an EventTarget
    //
    /*
     * wBM: if (!willBeModified && doc instanceof EventTarget) {
     * ((EventTarget) doc).addEventListener("DOMSubtreeModified", this,
     * false); }
     */
    return doc;
  }

  public void preload(String contentType, String docURI, boolean willBeModified) throws
      DOMTestIncompatibleException {
    if ("text/html".equals(contentType) ||
        "application/xhtml+xml".equals(contentType)) {
      if (docURI.startsWith("staff") || docURI.equals("datatype_normalization")) {
        throw DOMTestIncompatibleException.incompatibleLoad(docURI, contentType);
      }
    }
  }

  public Object createXPathEvaluator(Document doc) {
    return factory.createXPathEvaluator(doc);
  }

  public InputStream createStream(String bytes) throws DOMTestLoadException,
      IOException {
    int byteCount = bytes.length() / 2;
    byte[] array = new byte[byteCount];
    for (int i = 0; i < byteCount; i++) {
      array[i] = Byte.parseByte(bytes.substring(i * 2, i * 2 + 2), 16);
    }
    return new java.io.ByteArrayInputStream(array);
  }

  abstract public String getTargetURI();

  public final boolean isCoalescing() {
    return factory.isCoalescing();
  }

  public final boolean isExpandEntityReferences() {
    return factory.isExpandEntityReferences();
  }

  public final boolean isIgnoringElementContentWhitespace() {
    return factory.isIgnoringElementContentWhitespace();
  }

  public final boolean isNamespaceAware() {
    return factory.isNamespaceAware();
  }

  public final boolean isValidating() {
    return factory.isValidating();
  }

  public final boolean isSigned() {
    return true;
  }

  public final boolean isHasNullString() {
    return true;
  }

  public final String getContentType() {
    return factory.getContentType();
  }

  /**
   * Implementation of EventListener.handleEvent
   *
   * This method is called when a mutation is reported for a document that
   * was declared to not be modified during testing
   *
   * @param evt
   *            mutation event
   */
  /*
   * wBM: public final void handleEvent(Event evt) { mutationCount++; }
   */

  public final int getMutationCount() {
    return mutationCount;
  }

}
