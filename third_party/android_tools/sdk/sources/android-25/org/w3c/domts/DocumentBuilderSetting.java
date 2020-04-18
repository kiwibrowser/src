/*
 * Copyright (c) 2001-2004 World Wide Web Consortium, (Massachusetts Institute
 * of Technology, Institut National de Recherche en Informatique et en
 * Automatique, Keio University). All Rights Reserved. This program is
 * distributed under the W3C's Software Intellectual Property License. This
 * program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See W3C License
 * http://www.w3.org/Consortium/Legal/ for more details.
 */

package org.w3c.domts;

import javax.xml.parsers.DocumentBuilderFactory;

/**
 * This class is an parser setting, such as non-validating or entity-expanding.
 *
 * @author Curt Arnold @date 2 Feb 2002
 */
public final class DocumentBuilderSetting {
  /**
   * property name.
   */
  private final String property;

  /**
   *   property value.
   */
  private final boolean value;

  /**
   * strategy used to set or get property value.
   */
  private final DocumentBuilderSettingStrategy strategy;

  /**
   * coalescing = true.
   */
  public static final DocumentBuilderSetting coalescing =
      new DocumentBuilderSetting(
      "coalescing",
      true,
      DocumentBuilderSettingStrategy.coalescing);

  /**
   * coalescing = false.
   */
  public static final DocumentBuilderSetting notCoalescing =
      new DocumentBuilderSetting(
      "coalescing",
      false,
      DocumentBuilderSettingStrategy.coalescing);

  /**
   * expandEntityReferences = false.
   */
  public static final DocumentBuilderSetting expandEntityReferences =
      new DocumentBuilderSetting(
      "expandEntityReferences",
      true,
      DocumentBuilderSettingStrategy.expandEntityReferences);

  /**
   * expandEntityReferences = true.
   */
  public static final DocumentBuilderSetting notExpandEntityReferences =
      new DocumentBuilderSetting(
      "expandEntityReferences",
      false,
      DocumentBuilderSettingStrategy.expandEntityReferences);

  /**
   * ignoringElementContentWhitespace = true.
   */
  public static final DocumentBuilderSetting ignoringElementContentWhitespace =
      new DocumentBuilderSetting(
      "ignoringElementContentWhitespace",
      true,
      DocumentBuilderSettingStrategy.ignoringElementContentWhitespace);

  /**
   * ignoringElementContentWhitespace = false.
   */
  public static final DocumentBuilderSetting
      notIgnoringElementContentWhitespace =
      new DocumentBuilderSetting(
      "ignoringElementContentWhitespace",
      false,
      DocumentBuilderSettingStrategy.ignoringElementContentWhitespace);

  /**
   * namespaceAware = true.
   */
  public static final DocumentBuilderSetting namespaceAware =
      new DocumentBuilderSetting(
      "namespaceAware",
      true,
      DocumentBuilderSettingStrategy.namespaceAware);

  /**
   * namespaceAware = false.
   */
  public static final DocumentBuilderSetting notNamespaceAware =
      new DocumentBuilderSetting(
      "namespaceAware",
      false,
      DocumentBuilderSettingStrategy.namespaceAware);

  /**
   * validating = true.
   */
  public static final DocumentBuilderSetting validating =
      new DocumentBuilderSetting(
      "validating",
      true,
      DocumentBuilderSettingStrategy.validating);

  /**
   * validating = false.
   */
  public static final DocumentBuilderSetting notValidating =
      new DocumentBuilderSetting(
      "validating",
      false,
      DocumentBuilderSettingStrategy.validating);

  /**
   * signed = true.
   */
  public static final DocumentBuilderSetting signed =
      new DocumentBuilderSetting(
      "signed",
      true,
      DocumentBuilderSettingStrategy.signed);

  /**
   * signed = false.
   */
  public static final DocumentBuilderSetting notSigned =
      new DocumentBuilderSetting(
      "signed",
      false,
      DocumentBuilderSettingStrategy.signed);

  /**
   * hasNullString = true.
   */
  public static final DocumentBuilderSetting hasNullString =
      new DocumentBuilderSetting(
      "hasNullString",
      true,
      DocumentBuilderSettingStrategy.hasNullString);

  /**
   * hasNullString = false.
   */
  public static final DocumentBuilderSetting notHasNullString =
      new DocumentBuilderSetting(
      "hasNullString",
      false,
      DocumentBuilderSettingStrategy.hasNullString);

  /**
   * Schema validating enabled.
   */
  public static final DocumentBuilderSetting schemaValidating =
      new DocumentBuilderSetting(
      "schemaValidating",
      true,
      DocumentBuilderSettingStrategy.schemaValidating);

  /**
   * Schema validating disabled.
   */
  public static final DocumentBuilderSetting notSchemaValidating =
      new DocumentBuilderSetting(
      "schemaValidating",
      false,
      DocumentBuilderSettingStrategy.schemaValidating);

  /**
   * Comments ignored.
   */
  public static final DocumentBuilderSetting ignoringComments =
      new DocumentBuilderSetting(
      "ignoringComments",
      true,
      DocumentBuilderSettingStrategy.ignoringComments);

  /**
   * Comments preserved.
   */
  public static final DocumentBuilderSetting notIgnoringComments =
      new DocumentBuilderSetting(
      "ignoringComments",
      false,
      DocumentBuilderSettingStrategy.ignoringComments);

  /**
   * Protected constructor, use static members for supported settings.
   * @param property property name, follows JAXP.
   * @param value property value
   * @param strategy strategy, may not be null
   */
  protected DocumentBuilderSetting(
      String property,
      boolean value,
      DocumentBuilderSettingStrategy strategy) {
    if (property == null) {
      throw new NullPointerException("property");
    }
    this.property = property;
    this.value = value;
    this.strategy = strategy;
  }

  /**
   * Returns true if the settings have a conflict or are identical.
   *
   * @param other
   *            other setting, may not be null.
   * @return true if this setting and the specified setting conflict
   */
  public final boolean hasConflict(DocumentBuilderSetting other) {
    if (other == null) {
      throw new NullPointerException("other");
    }
    if (other == this) {
      return true;
    }
    return strategy.hasConflict(other.strategy);
  }

  /**
   * Determines current value of setting.
   * @param factory DOMTestDocumentBuilderFactory factory
   * @return boolean true if property enabled.
   */
  public final boolean hasSetting(DOMTestDocumentBuilderFactory factory) {
    return strategy.hasSetting(factory) == value;
  }

  /**
   * Attempts to change builder to have this setting.
   * @param factory DocumentBuilderFactory Factory for DOM builders
   * @throws DOMTestIncompatibleException
   *      if factory does not support the setting
   */
  public final void applySetting(DocumentBuilderFactory factory) throws
      DOMTestIncompatibleException {
    strategy.applySetting(factory, value);
  }

  /**
   * Gets the property name.
   * @return property name
   */
  public final String getProperty() {
    return property;
  }

  /**
   * Gets the property value.
   * @return property value
   */
  public final boolean getValue() {
    return value;
  }

  /**
   * Gets a string representation of the setting.
   * @return string representation
   */
  public final String toString() {
    StringBuffer builder = new StringBuffer(property);
    builder.append('=');
    builder.append(String.valueOf(value));
    return builder.toString();
  }

}
