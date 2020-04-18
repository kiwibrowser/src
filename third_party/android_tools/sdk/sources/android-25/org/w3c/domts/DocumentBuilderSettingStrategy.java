/*
 * Copyright (c) 2001-2004 World Wide Web Consortium, (Massachusetts Institute of
 * Technology, Institut National de Recherche en Informatique et en
 * Automatique, Keio University). All Rights Reserved. This program is
 * distributed under the W3C's Software Intellectual Property License. This
 * program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See W3C License
 * http://www.w3.org/Consortium/Legal/ for more details.
 */

package org.w3c.domts;

import java.lang.reflect.Method;

import javax.xml.parsers.DocumentBuilderFactory;

/**
 * This class is a strategy that provides the mapping from an abstract setting
 * (such as DocumentBuilderSetting.validating) to a specific DOM implementation
 *
 * @author Curt Arnold @date 2 Feb 2002
 */
public abstract class DocumentBuilderSettingStrategy {
  protected DocumentBuilderSettingStrategy() {
  }

  private static final String JAXP_SCHEMA_LANGUAGE =
      "http://java.sun.com/xml/jaxp/properties/schemaLanguage";
  private static final String W3C_XML_SCHEMA =
      "http://www.w3.org/2001/XMLSchema";

  public boolean hasConflict(DocumentBuilderSettingStrategy other) {
    return (other == this);
  }

  public abstract void applySetting(
      DocumentBuilderFactory factory,
      boolean value) throws DOMTestIncompatibleException;

  public abstract boolean hasSetting(DOMTestDocumentBuilderFactory factory);

  public static final DocumentBuilderSettingStrategy coalescing =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value)
        throws DOMTestIncompatibleException {
      factory.setCoalescing(value);
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      return factory.isCoalescing();
    }

  };

  public static final DocumentBuilderSettingStrategy
      expandEntityReferences =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value)
        throws DOMTestIncompatibleException {
      factory.setExpandEntityReferences(value);
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      return factory.isExpandEntityReferences();
    }
  };

  public static final DocumentBuilderSettingStrategy
      ignoringElementContentWhitespace =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value)
        throws DOMTestIncompatibleException {
      factory.setIgnoringElementContentWhitespace(value);
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      return factory.isIgnoringElementContentWhitespace();
    }
  };

  public static final DocumentBuilderSettingStrategy ignoringComments =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value)
        throws DOMTestIncompatibleException {
      if (value) {
        throw new DOMTestIncompatibleException(
            new Exception("ignoreComments=true not supported"),
            DocumentBuilderSetting.ignoringComments);
      }
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      return false;
    }
  };

  public static final DocumentBuilderSettingStrategy namespaceAware =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value) throws
        DOMTestIncompatibleException {
      factory.setNamespaceAware(value);
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      return factory.isNamespaceAware();
    }
  };

  public static final DocumentBuilderSettingStrategy validating =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value) throws
        DOMTestIncompatibleException {
      factory.setValidating(value);
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      return factory.isValidating();
    }
  };

  public static final DocumentBuilderSettingStrategy signed =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value) throws
        DOMTestIncompatibleException {
      if (!value) {
        throw new DOMTestIncompatibleException(
            null,
            DocumentBuilderSetting.notSigned);
      }
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      return true;
    }
  };

  public static final DocumentBuilderSettingStrategy hasNullString =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value) throws
        DOMTestIncompatibleException {
      if (!value) {
        throw new DOMTestIncompatibleException(
            null,
            DocumentBuilderSetting.notHasNullString);
      }
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      return true;
    }
  };

  public static final DocumentBuilderSettingStrategy schemaValidating =
      new DocumentBuilderSettingStrategy() {
    public void applySetting(DocumentBuilderFactory factory, boolean value) throws
        DOMTestIncompatibleException {
      if (value) {
        factory.setNamespaceAware(true);
        factory.setValidating(true);
        factory.setAttribute(JAXP_SCHEMA_LANGUAGE, W3C_XML_SCHEMA);
      }
      else {
        factory.setAttribute(JAXP_SCHEMA_LANGUAGE,
                             "http://www.w3.org/TR/REC-xml");
      }
    }

    public boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
      try {
        if (factory.isValidating()) {
          Method getAttrMethod = factory.getClass().getMethod("getAttribute",
              new Class[] {String.class});
          String val = (String) getAttrMethod.invoke(factory,
              new Object[] {JAXP_SCHEMA_LANGUAGE});
          return W3C_XML_SCHEMA.equals(val);
        }
      }
      catch (Exception ex) {
      }
      return false;
    }

    //
    //   schema validating conflicts with namespaceAware
    //        and validating
    //
    public boolean hasConflict(DocumentBuilderSettingStrategy other) {
      if (other == this ||
          other == DocumentBuilderSettingStrategy.namespaceAware ||
          other == DocumentBuilderSettingStrategy.validating) {
        return true;
      }
      return false;
    }

  };

}
