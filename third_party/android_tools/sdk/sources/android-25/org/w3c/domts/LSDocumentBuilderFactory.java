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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.Document;

/**
 *   This class implements the generic parser and configuation
 *   abstract class for the DOM L3 implementations
 *
 *   @author Curt Arnold
 */
public class LSDocumentBuilderFactory
    extends DOMTestDocumentBuilderFactory {

  private final Object parser;
  private final Method parseURIMethod;
  private final DOMImplementation impl;

  /**
   *
   * Abstract class for a strategy to map a DocumentBuilderSetting
   * to an action on LSParser.
   */
  private static abstract class LSStrategy {

    /**
     * Constructor.
     */
    protected LSStrategy() {
    }

    /**
     * Applies setting to LSParser
     *
     * @param setting setting
     * @param parser parser
     * @throws DOMTestIncompatibleException if parser does not support setting
     */
    public abstract void applySetting(DocumentBuilderSetting setting,
                                      Object parser) throws
        DOMTestIncompatibleException;

    /**
     * Gets state of setting for parser
     *
     * @param parser parser
     * @return state of setting
     */
    public abstract boolean hasSetting(Object parser);

  }

  /**
   * Represents a fixed setting, for example, all Java implementations
   * supported signed values.
   *
   */
  private static class LSFixedStrategy
      extends LSStrategy {
    private final boolean fixedValue;

    /**
     * Constructor
     *
     * @param settingName setting name
     * @param fixedValue fixed value
     */
    public LSFixedStrategy(boolean fixedValue) {
      this.fixedValue = fixedValue;
    }

    /**
     * Apply setting.  Throws exception if requested setting
     * does not match fixed value.
     */
    public void applySetting(DocumentBuilderSetting setting, Object parser) throws
        DOMTestIncompatibleException {
      if (setting.getValue() != fixedValue) {
        throw new DOMTestIncompatibleException(null, setting);
      }
    }

    /**
     * Gets fixed value for setting
     */
    public boolean hasSetting(Object parser) {
      return fixedValue;
    }
  }

  /**
   * A strategy for a setting that can be applied by setting a DOMConfiguration
   * parameter.
   *
   */
  private static class LSParameterStrategy
      extends LSStrategy {
    private final String lsParameter;
    private final boolean inverse;

    /**
     * Constructor
     *
     * @param lsParameter corresponding DOMConfiguration parameter
     * @param inverse if true, DOMConfiguration value is the inverse
     * of the setting value
     */
    public LSParameterStrategy(String lsParameter, boolean inverse) {
      this.lsParameter = lsParameter;
      this.inverse = inverse;
    }

    protected static void setParameter(DocumentBuilderSetting setting,
                                       Object parser,
                                       String parameter,
                                       Object value) throws
        DOMTestIncompatibleException {
      try {
        Method domConfigMethod = parser.getClass().getMethod("getDomConfig",
            new Class[0]);
        Object domConfig = domConfigMethod.invoke(parser, new Object[0]);
        Method setParameterMethod = domConfig.getClass().getMethod(
            "setParameter", new Class[] {String.class, Object.class});
        setParameterMethod.invoke(domConfig, new Object[] {parameter, value});

      }
      catch (InvocationTargetException ex) {
        throw new DOMTestIncompatibleException(ex.getTargetException(), setting);
      }
      catch (Exception ex) {
        throw new DOMTestIncompatibleException(ex, setting);
      }
    }

    protected static Object getParameter(Object parser,
                                         String parameter) throws Exception {
      Method domConfigMethod = parser.getClass().getMethod("getDomConfig",
          new Class[0]);
      Object domConfig = domConfigMethod.invoke(parser, new Object[0]);
      Method getParameterMethod = domConfig.getClass().getMethod("getParameter",
          new Class[] {String.class});
      return getParameterMethod.invoke(domConfig, new Object[] {parameter});
    }

    /**
     * Apply setting
     */
    public void applySetting(DocumentBuilderSetting setting, Object parser) throws
        DOMTestIncompatibleException {
      if (inverse) {
        setParameter(setting, parser, lsParameter,
                     new Boolean(!setting.getValue()));
      }
      else {
        setParameter(setting, parser, lsParameter, new Boolean(setting.getValue()));
      }
    }

    /**
     * Get value of setting
     */
    public boolean hasSetting(Object parser) {
      try {
        if (inverse) {
          return! ( (Boolean) getParameter(parser, lsParameter)).booleanValue();
        }
        else {
          return ( (Boolean) getParameter(parser, lsParameter)).booleanValue();
        }
      }
      catch (Exception ex) {
        return false;
      }
    }
  }

  /**
   * A strategy for the validation settings which require
   * two DOMConfigurure parameters being set, 'validate' and 'schema-type'
   *
   */
  private static class LSValidateStrategy
      extends LSParameterStrategy {
    private final String schemaType;

    /**
     * Constructor
     * @param schemaType schema type
     */
    public LSValidateStrategy(String schemaType) {
      super("validate", false);
      this.schemaType = schemaType;
    }

    /**
     * Apply setting
     */
    public void applySetting(DocumentBuilderSetting setting, Object parser) throws
        DOMTestIncompatibleException {
      super.applySetting(setting, parser);
      setParameter(null, parser, "schema-type", schemaType);
    }

    /**
     * Get setting value
     */
    public boolean hasSetting(Object parser) {
      if (super.hasSetting(parser)) {
        try {
          String parserSchemaType = (String) getParameter(parser, "schema-type");
          if (schemaType == null || schemaType.equals(parserSchemaType)) {
            return true;
          }
        }
        catch (Exception ex) {
        }
      }
      return false;
    }

  }

  /**
   * Strategies for mapping DocumentBuilderSettings to
   * actions on LSParser
   */
  private static final Map strategies;

  static {
    strategies = new HashMap();
    strategies.put("coalescing", new LSParameterStrategy("cdata-sections", true));
    strategies.put("expandEntityReferences", new LSParameterStrategy("entities", true));
    strategies.put("ignoringElementContentWhitespace",
                   new LSParameterStrategy("element-content-whitespace", true));
    strategies.put("namespaceAware", new LSParameterStrategy("namespaces", false));
    strategies.put("validating",
                   new LSValidateStrategy("http://www.w3.org/TR/REC-xml"));
    strategies.put("schemaValidating",
                   new LSValidateStrategy("http://www.w3.org/2001/XMLSchema"));
    strategies.put("ignoringComments", new LSParameterStrategy("comments", true));
    strategies.put("signed", new LSFixedStrategy(true));
    strategies.put("hasNullString", new LSFixedStrategy(true));
  }

  /**
   * Creates a LS implementation of DOMTestDocumentBuilderFactory.
   * @param settings array of settings, may be null.
   * @throws DOMTestIncompatibleException
   *     Thrown if implementation does not support the specified settings
   */
  public LSDocumentBuilderFactory(DocumentBuilderSetting[] settings) throws
      DOMTestIncompatibleException {
    super(settings);

    try {
      Class domImplRegistryClass = Class.forName(
          "org.w3c.dom.bootstrap.DOMImplementationRegistry");
      Method newInstanceMethod = domImplRegistryClass.getMethod("newInstance", (Class<?>) null);
      Object domRegistry = newInstanceMethod.invoke(null, (Class<?>) null);
      Method getDOMImplementationMethod = domImplRegistryClass.getMethod(
          "getDOMImplementation", new Class[] {String.class});
      impl = (DOMImplementation) getDOMImplementationMethod.invoke(domRegistry,
          new Object[] {"LS"});
      Method createLSParserMethod = impl.getClass().getMethod("createLSParser",
          new Class[] {short.class, String.class});
      parser = createLSParserMethod.invoke(impl,
                                           new Object[] {new Short( (short) 1), null});
      parseURIMethod = parser.getClass().getMethod("parseURI",
          new Class[] {String.class});
    }
    catch (InvocationTargetException ex) {
      throw new DOMTestIncompatibleException(ex.getTargetException(), null);
    }
    catch (Exception ex) {
      throw new DOMTestIncompatibleException(ex, null);
    }

    if (settings != null) {
      for (int i = 0; i < settings.length; i++) {
        Object strategy = strategies.get(settings[i].getProperty());
        if (strategy == null) {
          throw new DOMTestIncompatibleException(null, settings[i]);
        }
        else {
          ( (LSStrategy) strategy).applySetting(settings[i], parser);
        }
      }
    }
  }

  /**
   *    Create new instance of document builder factory
   *    reflecting specified settings
   *    @param newSettings new settings
   *    @return New instance
   *    @throws DOMTestIncompatibleException
   *         if settings are not supported by implementation
   */
  public DOMTestDocumentBuilderFactory newInstance(
      DocumentBuilderSetting[] newSettings) throws DOMTestIncompatibleException {
    if (newSettings == null) {
      return this;
    }
    DocumentBuilderSetting[] mergedSettings = mergeSettings(newSettings);
    return new LSDocumentBuilderFactory(mergedSettings);
  }

  /**
   *    Loads specified URL
   *    @param url url to load
   *    @return DOM document
   *    @throws DOMTestLoadException if unable to load document
   */
  public Document load(java.net.URL url) throws DOMTestLoadException {
    try {
      return (Document) parseURIMethod.invoke(parser,
                                              new Object[] {url.toString()});
    }
    catch (InvocationTargetException ex) {
      throw new DOMTestLoadException(ex.getTargetException());
    }
    catch (Exception ex) {
      throw new DOMTestLoadException(ex);
    }
  }

  /**
   *     Gets DOMImplementation
   *     @return DOM implementation, may be null
   */
  public DOMImplementation getDOMImplementation() {
    return impl;
  }

  /**
   *   Determines if the implementation supports the specified feature
   *   @param feature Feature
   *   @param version Version
   *   @return true if implementation supports the feature
   */
  public boolean hasFeature(String feature, String version) {
    return getDOMImplementation().hasFeature(feature, version);
  }

  private boolean hasProperty(String parameter) {
    try {
      return ( (Boolean) LSParameterStrategy.getParameter(parser, parameter)).
          booleanValue();
    }
    catch (Exception ex) {
      return true;
    }

  }

  /**
   *   Indicates whether the implementation combines text and cdata nodes.
   *   @return true if coalescing
   */
  public boolean isCoalescing() {
    return!hasProperty("cdata-sections");
  }

  /**
   *   Indicates whether the implementation expands entity references.
   *   @return true if expanding entity references
   */
  public boolean isExpandEntityReferences() {
    return!hasProperty("entities");
  }

  /**
   *   Indicates whether the implementation ignores
   *       element content whitespace.
   *   @return true if ignoring element content whitespace
   */
  public boolean isIgnoringElementContentWhitespace() {
    return!hasProperty("element-content-whitespace");
  }

  /**
   *   Indicates whether the implementation is namespace aware.
   *   @return true if namespace aware
   */
  public boolean isNamespaceAware() {
    return hasProperty("namespaces");
  }

  /**
   *   Indicates whether the implementation is validating.
   *   @return true if validating
   */
  public boolean isValidating() {
    return hasProperty("validate");
  }

}
