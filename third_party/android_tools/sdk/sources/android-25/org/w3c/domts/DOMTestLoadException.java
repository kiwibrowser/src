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
 * Encapsulates a concrete load exception such as
 * a SAX exception
 * @author Curt Arnold
 * @date 2 Feb 2002
 */
public class DOMTestLoadException
    extends Exception {
  private final Throwable innerException;

  /**
   * Constructor
   * @param innerException should not be null
   */
  public DOMTestLoadException(Throwable innerException) {
    this.innerException = innerException;
  }

  public String toString() {
    if (innerException != null) {
      return innerException.toString();
    }
    return super.toString();
  }
}
