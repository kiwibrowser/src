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

import java.util.ArrayList;
import java.util.List;
import java.util.Iterator;

import org.w3c.dom.DOMError;
import org.w3c.dom.DOMErrorHandler;

/**
 *   This is a utility implementation of EventListener
 *      that captures all events and provides access
 *      to lists of all events by mode
 */
public class DOMErrorMonitor
    implements DOMErrorHandler {
  private final List errors = new ArrayList();

  /**
   * Public constructor
   *
   */
  public DOMErrorMonitor() {
  }

  /**
   * Implementation of DOMErrorHandler.handleError that
   * adds copy of error to list for later retrieval.
   *
   */
  public boolean handleError(DOMError error) {
    errors.add(new DOMErrorImpl(error));
    return true;
  }

  /**
   * Gets list of errors
   *
   * @return return errors
   */
  public List getAllErrors() {
    return new ArrayList(errors);
  }

  public void assertLowerSeverity(DOMTestCase testCase, String id, int severity) {
    Iterator iter = errors.iterator();
    while (iter.hasNext()) {
      DOMError error = (DOMError) iter.next();
      if (error.getSeverity() >= severity) {
        testCase.fail(id + error.getMessage());
      }
    }
  }
}
