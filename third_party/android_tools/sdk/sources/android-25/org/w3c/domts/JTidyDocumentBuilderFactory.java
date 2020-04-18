/*
 * Copyright (c) 2002-2004 World Wide Web Consortium,
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

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.Document;

/**
 *   This class implements the generic parser builder
 *   for JTidy (http://sf.net/projects/JTidy) which reads HTML
 *   and supports the fundamental DOM interfaces but not either HTML L1 DOM
 *   or HTML L2 DOM
 */
public class JTidyDocumentBuilderFactory
    extends DOMTestDocumentBuilderFactory {

  private final Constructor tidyConstructor;
  private final Method parseDOMMethod;
  private final DOMImplementation domImpl;
  private static final Class[] NO_CLASSES = new Class[0];
  private static final Object[] NO_OBJECTS = new Object[0];

  /**
   * Creates a implementation of DOMTestDocumentBuilderFactory
   * using JTidy's HTML parser and DOM implementation
   * @param settings array of settings, may be null.
   */
  public JTidyDocumentBuilderFactory(
      DocumentBuilderSetting[] settings) throws DOMTestIncompatibleException {
    super(settings);

    try {
      ClassLoader classLoader = ClassLoader.getSystemClassLoader();
      Class tidyClass = classLoader.loadClass("org.w3c.tidy.Tidy");

      tidyConstructor =
          tidyClass.getConstructor(NO_CLASSES);

      parseDOMMethod =
          tidyClass.getMethod("parseDOM",
                              new Class[] {java.io.InputStream.class,
                              java.io.OutputStream.class});

      //
      //  JTidy doesn't implement DOMImplementation so
      //    we will do it here
      domImpl = new JTidyDOMImplementation();

    }
    catch (Exception ex) {
      throw new DOMTestIncompatibleException(ex, null);
    }

    //
    //    apply settings to selected document builder
    //         may throw exception if incompatible
    if (settings != null) {
      for (int i = 0; i < settings.length; i++) {
//        settings[i].applySetting(factory);
      }
    }
  }

  public DOMTestDocumentBuilderFactory newInstance(DocumentBuilderSetting[]
      newSettings) throws DOMTestIncompatibleException {
    if (newSettings == null) {
      return this;
    }
    DocumentBuilderSetting[] mergedSettings = mergeSettings(newSettings);
    return new JTidyDocumentBuilderFactory(mergedSettings);
  }

  public Document load(java.net.URL url) throws DOMTestLoadException {
    Document doc = null;
    try {
      java.io.InputStream stream = url.openStream();
      Object tidyObj = tidyConstructor.newInstance(new Object[0]);
      doc = (Document) parseDOMMethod.invoke(tidyObj,
                                             new Object[] {stream, null});
    }
    catch (InvocationTargetException ex) {
      throw new DOMTestLoadException(ex.getTargetException());
    }
    catch (Exception ex) {
      throw new DOMTestLoadException(ex);
    }
    return doc;
  }

  public DOMImplementation getDOMImplementation() {
    return domImpl;
  }

  public boolean hasFeature(String feature, String version) {
    return domImpl.hasFeature(feature, version);
  }

  public String getContentType() {
    return "text/html";
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
    return false;
  }

  public boolean isValidating() {
    return false;
  }
}
