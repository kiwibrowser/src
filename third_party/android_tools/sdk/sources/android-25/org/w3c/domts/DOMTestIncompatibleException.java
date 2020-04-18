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


/**
 * This exception represents a mismatch between the
 * requirements of the test (for example, entity preserving)
 * and the capabilities of the parser under test.
 * @author Curt Arnold
 */
public class DOMTestIncompatibleException
    extends Exception {
  private final String msg;

  private DOMTestIncompatibleException(String msg) {
    this.msg = msg;
  }

  /**
   *  Constructor from a ParserConfigurationException
   *  or reflection exception
   */
  public DOMTestIncompatibleException(Throwable ex,
                                      DocumentBuilderSetting setting) {
    if (ex != null) {
      msg = ex.toString();
    }
    else {
      if (setting != null) {
        msg = setting.toString();
      }
      else {
        msg = super.toString();
      }
    }
  }

  public static DOMTestIncompatibleException incompatibleFeature(String feature,
      String version) {
    StringBuffer buf = new StringBuffer(
        "Implementation does not support feature \"");
    buf.append(feature);
    buf.append("\" version=\"");
    buf.append(version);
    buf.append("\".");
    return new DOMTestIncompatibleException(buf.toString());
  }

  public static DOMTestIncompatibleException incompatibleLoad(String href,
      String contentType) {
    StringBuffer buf = new StringBuffer(
        "Document is incompatible with content type, \"");
    buf.append(href);
    buf.append("\" not available for =\"");
    buf.append(contentType);
    buf.append("\".");
    return new DOMTestIncompatibleException(buf.toString());
  }

  public String toString() {
    return msg;
  }

}
