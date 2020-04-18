/*
 * Copyright (c) 2004 World Wide Web Consortium,
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

import org.w3c.dom.DOMError;
import org.w3c.dom.DOMLocator;

/**
 *   This is a utility implementation of EventListener
 *      that captures all events and provides access
 *      to lists of all events by mode
 */
public class DOMErrorImpl
    implements DOMError {
  private final short severity;
  private final String message;
  private final String type;
  private final Object relatedException;
  private final Object relatedData;
  private final DOMLocator location;

  /**
   * Public constructor
   *
   */
  public DOMErrorImpl(DOMError src) {
    this.severity = src.getSeverity();
    this.message = src.getMessage();
    this.type = src.getType();
    this.relatedException = src.getRelatedException();
    this.relatedData = src.getRelatedData();
    this.location = new DOMLocatorImpl(src.getLocation());
  }

  public final short getSeverity() {
    return severity;
  }

  public final String getMessage() {
    return message;
  }

  public final String getType() {
    return type;
  }

  public final Object getRelatedException() {
    return relatedException;
  }

  public final Object getRelatedData() {
    return relatedData;
  }

  public final DOMLocator getLocation() {
    return location;
  }
}
